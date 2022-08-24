/*
 * Asp engine tree implementation.
 */

#include "tree.h"
#include "data.h"
#include "compare.h"

static AspRunResult Insert
    (AspEngine *, AspDataEntry *tree, AspDataEntry *node);
static AspDataEntry *FindNode
    (AspEngine *, AspDataEntry *tree, AspDataEntry *keyNode);
static AspDataEntry *Min
    (AspEngine *, AspDataEntry *tree, AspDataEntry *node);
static AspRunResult Shift
    (AspEngine *, AspDataEntry *tree,
     AspDataEntry *node1, AspDataEntry *node2);
static int CompareKeys
    (AspEngine *, const AspDataEntry *tree,
     const AspDataEntry *leftNode, const AspDataEntry *rightNode);

static AspRunResult SetLeftIndex
    (AspEngine *, AspDataEntry *node, uint32_t index);
static uint32_t GetLeftIndex(AspEngine *, AspDataEntry *node);
static AspRunResult SetRightIndex
    (AspEngine *, AspDataEntry *node, uint32_t index);
static uint32_t GetRightIndex(AspEngine *, AspDataEntry *node);
static void PruneLinks(AspEngine *, AspDataEntry *node);
static bool IsTreeType(DataType type);
static bool IsNodeType(DataType type);
static AspRunResult NotFoundResult(AspDataEntry *tree);

AspTreeResult AspTreeInsert
    (AspEngine *engine, AspDataEntry *tree,
     AspDataEntry *key, AspDataEntry *value)
{
    AspTreeResult result = {AspRunResult_OK, 0, 0, 0, false};

    result.result = AspAssert(engine, tree != 0);
    if (result.result != AspRunResult_OK)
        return result;

    uint8_t treeType = AspDataGetType(tree);
    AspAssert
        (engine,
         treeType == DataType_Set || treeType == DataType_Dictionary);
    AspAssert(engine, key != 0 && AspIsObject(key));
    result.result = AspAssert
        (engine,
         treeType == DataType_Dictionary ?
         value != 0 && AspIsObject(value) : value == 0);
    if (result.result != AspRunResult_OK)
        return result;

    /* Allocate a node entry and link it to the given key. */
    result.node = AspAllocEntry
        (engine,
         treeType == DataType_Dictionary ?
         DataType_DictionaryNode : DataType_SetNode);
    if (result.node == 0)
    {
        result.result = AspRunResult_OutOfDataMemory;
        return result;
    }
    AspDataSetTreeNodeKeyIndex(result.node, AspIndex(engine, key));

    /* Determine whether the key already exists. */
    AspDataEntry *foundNode = FindNode(engine, tree, result.node);
    if (foundNode != 0)
    {
        AspUnref(engine, result.node);
        result.node = foundNode;
        result.key = AspValueEntry
            (engine, AspDataGetTreeNodeKeyIndex(foundNode));

        /* Replace the entry's value if applicable. */
        if (treeType != DataType_Set)
        {
            AspUnref(engine, AspValueEntry
                (engine, AspDataGetTreeNodeValueIndex(foundNode)));
            AspDataSetTreeNodeValueIndex(foundNode, AspIndex
                (engine, value));
            result.value = AspValueEntry
                (engine, AspDataGetTreeNodeValueIndex(foundNode));
            AspRef(engine, result.value);
        }

        return result;
    }

    /* Link the new node to the value if applicable. */
    AspRef(engine, key);
    result.key = key;
    AspDataSetTreeNodeValueIndex(result.node, AspIndex(engine, value));
    if (treeType == DataType_Dictionary)
    {
        AspRef(engine, value);
        result.value = value;
    }
    result.inserted = true;

    result.result = Insert(engine, tree, result.node);

    return result;
}

AspTreeResult AspTreeTryInsertBySymbol
    (AspEngine *engine, AspDataEntry *tree,
     int32_t symbol, AspDataEntry *value)
{
    AspTreeResult result = {AspRunResult_OK, 0, 0, 0, false};

    AspAssert(engine, tree != 0);
    AspAssert(engine, AspDataGetType(tree) == DataType_Namespace);
    result.result = AspAssert
        (engine, value != 0 && AspIsObject(value));
    if (result.result != AspRunResult_OK)
        return result;

    /* Determine whether the symbol already exists. */
    result = AspFindSymbol(engine, tree, symbol);
    if (result.result != AspRunResult_OK || result.node != 0)
        return result;

    /* Allocate a node entry, assign it the given symbol, and link it to
       the given value. */
    result.node = AspAllocEntry(engine, DataType_NamespaceNode);
    if (result.node == 0)
    {
        result.result = AspRunResult_OutOfDataMemory;
        return result;
    }
    AspDataSetNamespaceNodeSymbol(result.node, symbol);
    AspRef(engine, value);
    AspDataSetTreeNodeValueIndex(result.node, AspIndex(engine, value));
    result.value = value;
    result.inserted = true;

    result.result = Insert(engine, tree, result.node);

    return result;
}

AspRunResult AspTreeEraseNode
    (AspEngine *engine, AspDataEntry *tree, AspDataEntry *keyNode,
     bool eraseKey, bool eraseValue)
{
    AspRunResult result = AspRunResult_OK;

    AspAssert
        (engine, tree != 0 && IsTreeType(AspDataGetType(tree)));
    result = AspAssert
        (engine, keyNode != 0 && IsNodeType(AspDataGetType(keyNode)));
    if (result != AspRunResult_OK)
        return result;

    AspDataEntry *entry = FindNode(engine, tree, keyNode);
    if (entry == 0)
        return NotFoundResult(tree);

    uint32_t leftIndex = GetLeftIndex(engine, entry);
    uint32_t rightIndex = GetRightIndex(engine, entry);
    if (leftIndex == 0)
    {
        AspDataEntry *rightNode = AspEntry(engine, rightIndex);
        result = Shift(engine, tree, entry, rightNode);
    }
    else if (rightIndex == 0)
    {
        AspDataEntry *leftNode = AspEntry(engine, leftIndex);
        result = Shift(engine, tree, entry, leftNode);
    }
    else
    {
        AspTreeResult nextResult = AspTreeNext(engine, tree, entry);
        AspDataEntry *nextNode = nextResult.node;
        if (AspDataGetTreeNodeParentIndex(nextNode) != AspIndex(engine, entry))
        {
            AspDataEntry *rightNode = AspEntry
                (engine, GetRightIndex(engine, nextNode));
            result = Shift(engine, tree, nextNode, rightNode);
            if (result != AspRunResult_OK)
                return result;
            result = SetRightIndex
                (engine, nextNode, GetRightIndex(engine, entry));
            if (result != AspRunResult_OK)
                return result;
            rightNode = AspEntry(engine, GetRightIndex(engine, nextNode));
            AspDataSetTreeNodeParentIndex
                (rightNode, AspIndex(engine, nextNode));
        }

        result = Shift(engine, tree, entry, nextNode);
        if (result != AspRunResult_OK)
            return result;
        result = SetLeftIndex
            (engine, nextNode, GetLeftIndex(engine, entry));
        if (result != AspRunResult_OK)
            return result;
        AspDataEntry *leftNode = AspEntry
            (engine, GetLeftIndex(engine, nextNode));
        AspDataSetTreeNodeParentIndex
            (leftNode, AspIndex(engine, nextNode));
    }

    if (result != AspRunResult_OK)
        return result;

    uint8_t type = AspDataGetType(entry);
    if (eraseKey && type != DataType_NamespaceNode)
        AspUnref(engine, AspValueEntry
            (engine, AspDataGetTreeNodeKeyIndex(entry)));
    if (eraseValue && type != DataType_SetNode)
        AspUnref(engine, AspValueEntry
            (engine, AspDataGetTreeNodeValueIndex(entry)));

    if (type != DataType_SetNode && AspDataGetTreeNodeLinksIndex(entry) != 0)
        AspUnref(engine, AspValueEntry
            (engine, AspDataGetTreeNodeLinksIndex(entry)));

    AspUnref(engine, entry);
    AspDataSetTreeCount(tree, AspDataGetTreeCount(tree) - 1U);

    return result;
}

AspTreeResult AspTreeFind
    (AspEngine *engine, AspDataEntry *tree, const AspDataEntry *key)
{
    AspTreeResult result = {AspRunResult_OK, 0, 0, 0, false};

    result.result = AspAssert(engine, tree != 0);
    if (result.result != AspRunResult_OK)
        return result;

    uint8_t treeType = AspDataGetType(tree);
    AspAssert
        (engine,
         treeType == DataType_Set || treeType == DataType_Dictionary);
    result.result = AspAssert(engine, key != 0 && AspIsObject(key));
    if (result.result != AspRunResult_OK)
        return result;

    AspDataEntry *keyNode = AspAllocEntry
        (engine,
         treeType == DataType_Dictionary ?
         DataType_DictionaryNode : DataType_SetNode);
    if (keyNode == 0)
    {
        result.result = AspRunResult_OutOfDataMemory;
        return result;
    }
    AspDataSetTreeNodeKeyIndex(keyNode, AspIndex(engine, key));
    result.node = FindNode(engine, tree, keyNode);
    if (result.node != 0 && treeType != DataType_Set)
        result.value = AspValueEntry
            (engine, AspDataGetTreeNodeValueIndex(result.node));
    AspUnref(engine, keyNode);

    return result;
}

AspTreeResult AspFindSymbol
    (AspEngine *engine, AspDataEntry *tree, int32_t symbol)
{
    AspTreeResult result = {AspRunResult_OK, 0, 0, 0, false};

    AspAssert(engine, tree != 0);
    result.result = AspAssert
        (engine, AspDataGetType(tree) == DataType_Namespace);
    if (result.result != AspRunResult_OK)
        return result;

    AspDataEntry *keyNode = AspAllocEntry(engine, DataType_NamespaceNode);
    if (keyNode == 0)
    {
        result.result = AspRunResult_OutOfDataMemory;
        return result;
    }
    AspDataSetNamespaceNodeSymbol(keyNode, symbol);
    result.node = FindNode(engine, tree, keyNode);
    if (result.node != 0)
        result.value = AspValueEntry
            (engine, AspDataGetTreeNodeValueIndex(result.node));
    AspUnref(engine, keyNode);

    return result;
}

AspTreeResult AspTreeNext
    (AspEngine *engine, AspDataEntry *tree, AspDataEntry *node)
{
    AspTreeResult result = {AspRunResult_OK, 0, 0, 0, false};

    AspAssert(engine, tree != 0);
    AspAssert(engine, IsTreeType(AspDataGetType(tree)));
    result.result = AspAssert
        (engine, node == 0 || IsNodeType(AspDataGetType(node)));
    if (result.result != AspRunResult_OK)
        return result;

    uint32_t rootIndex = AspDataGetTreeRootIndex(tree);
    if (rootIndex == 0)
        return result;

    result.node = node;
    if (result.node == 0)
    {
        AspDataEntry *rootNode = AspEntry(engine, rootIndex);
        result.node = Min(engine, tree, rootNode);
    }
    else if (GetRightIndex(engine, result.node) != 0)
    {
        AspDataEntry *rightNode = AspEntry
            (engine, GetRightIndex(engine, result.node));
        result.node = Min(engine, tree, rightNode);
    }
    else
    {
        uint32_t parentIndex = AspDataGetTreeNodeParentIndex(node);
        result.node = AspEntry(engine, parentIndex);
        while (result.node != 0 &&
               AspIndex(engine, node) == GetRightIndex(engine, result.node))
        {
            node = result.node;
            parentIndex = AspDataGetTreeNodeParentIndex(result.node);
            result.node = AspEntry(engine, parentIndex);
        }
    }

    if (result.node != 0)
    {
        if (AspDataGetType(tree) != DataType_Namespace)
            result.key = AspValueEntry
                (engine, AspDataGetTreeNodeKeyIndex(result.node));
        if (AspDataGetType(tree) != DataType_Set)
            result.value = AspValueEntry
                (engine, AspDataGetTreeNodeValueIndex(result.node));
    }

    return result;
}

AspDataEntry *AspTreeNodeValue(AspEngine *engine, AspDataEntry *node)
{
    return node == 0 || AspDataGetType(node) == DataType_SetNode ?
        0 : AspValueEntry(engine, AspDataGetTreeNodeValueIndex(node));
}

static AspRunResult Insert
    (AspEngine *engine, AspDataEntry *tree, AspDataEntry *node)
{
    AspRunResult result = AspRunResult_OK;

    AspAssert
        (engine, tree != 0 && IsTreeType(AspDataGetType(tree)));
    result = AspAssert
        (engine, node != 0 && IsNodeType(AspDataGetType(node)));
    if (result != AspRunResult_OK)
        return result;

    AspDataEntry *parentNode = 0;
    AspDataEntry *targetNode = AspEntry(engine, AspDataGetTreeRootIndex(tree));
    while (targetNode != 0)
    {
        parentNode = targetNode;
        uint32_t childIndex =
            CompareKeys(engine, tree, node, targetNode) < 0 ?
            GetLeftIndex(engine, targetNode) :
            GetRightIndex(engine, targetNode);
        targetNode = AspEntry(engine, childIndex);
    }

    AspDataSetTreeNodeParentIndex(node, AspIndex(engine, parentNode));
    uint32_t nodeIndex = AspIndex(engine, node);
    if (parentNode == 0)
        AspDataSetTreeRootIndex(tree, nodeIndex);
    else if (CompareKeys(engine, tree, node, parentNode) < 0)
        result = SetLeftIndex(engine, parentNode, nodeIndex);
    else
        result = SetRightIndex(engine, parentNode, nodeIndex);
    if (result != AspRunResult_OK)
        return result;

    AspDataSetTreeCount(tree, AspDataGetTreeCount(tree) + 1U);

    return result;
}

static AspDataEntry *FindNode
    (AspEngine *engine, AspDataEntry *tree, AspDataEntry *keyNode)
{
    AspAssert
        (engine, tree != 0 && IsTreeType(AspDataGetType(tree)));
    AspRunResult assertResult = AspAssert
        (engine, keyNode != 0 && IsNodeType(AspDataGetType(keyNode)));
    if (assertResult != AspRunResult_OK)
        return 0;

    AspDataEntry *node = AspEntry(engine, AspDataGetTreeRootIndex(tree));
    while (node != 0 && CompareKeys(engine, tree, keyNode, node) != 0)
        node = AspEntry(engine,
            CompareKeys(engine, tree, keyNode, node) < 0 ?
            GetLeftIndex(engine, node) : GetRightIndex(engine, node));

    return node;
}

static AspDataEntry *Min
    (AspEngine *engine, AspDataEntry *tree, AspDataEntry *node)
{
    AspAssert
        (engine, tree != 0 && IsTreeType(AspDataGetType(tree)));
    AspRunResult assertResult = AspAssert
        (engine, node != 0 && IsNodeType(AspDataGetType(node)));
    if (assertResult != AspRunResult_OK)
        return 0;

    while (GetLeftIndex(engine, node) != 0)
        node = AspEntry(engine, GetLeftIndex(engine, node));

    return node;
}

static AspRunResult Shift
    (AspEngine *engine, AspDataEntry *tree,
     AspDataEntry *node1, AspDataEntry *node2)
{
    AspRunResult result = AspRunResult_OK;

    AspAssert
        (engine, tree != 0 && IsTreeType(AspDataGetType(tree)));
    AspAssert
        (engine, node1 != 0 && IsNodeType(AspDataGetType(node1)));
    result = AspAssert
        (engine, node2 == 0 || IsNodeType(AspDataGetType(node2)));
    if (result != AspRunResult_OK)
        return 0;

    uint32_t node1ParentIndex = AspDataGetTreeNodeParentIndex(node1);
    if (node1ParentIndex == 0)
        AspDataSetTreeRootIndex(tree, AspIndex(engine, node2));
    else
    {
        AspDataEntry *parentNode = AspEntry(engine, node1ParentIndex);
        uint32_t index2 = AspIndex(engine, node2);
        if (AspIndex(engine, node1) == GetLeftIndex(engine, parentNode))
            result = SetLeftIndex(engine, parentNode, index2);
        else
            result = SetRightIndex(engine, parentNode, index2);
    }
    if (result != AspRunResult_OK)
        return result;

    if (node2 != 0)
        AspDataSetTreeNodeParentIndex
            (node2, AspDataGetTreeNodeParentIndex(node1));

    return result;
}

static int CompareKeys
    (AspEngine *engine, const AspDataEntry *tree,
     const AspDataEntry *leftNode, const AspDataEntry *rightNode)
{
    AspAssert
        (engine, tree != 0 && IsTreeType(AspDataGetType(tree)));
    AspAssert
        (engine, leftNode != 0 && IsNodeType(AspDataGetType(leftNode)));
    AspRunResult assertResult = AspAssert
        (engine, rightNode != 0 && IsNodeType(AspDataGetType(rightNode)));
    if (assertResult != AspRunResult_OK)
        return 0;

    /* For namespaces, compare the symbols. Otherwise, compare objects. */
    if (AspDataGetType(tree) == DataType_Namespace)
    {
        int32_t leftSymbol = AspDataGetNamespaceNodeSymbol(leftNode);
        int32_t rightSymbol = AspDataGetNamespaceNodeSymbol(rightNode);
        return
            leftSymbol == rightSymbol ? 0 :
            leftSymbol < rightSymbol ? -1 : 1;
    }
    else
        return AspCompare
            (engine,
             AspValueEntry(engine, AspDataGetTreeNodeKeyIndex(leftNode)),
             AspValueEntry(engine, AspDataGetTreeNodeKeyIndex(rightNode)));
}

static AspRunResult SetLeftIndex
    (AspEngine *engine, AspDataEntry *node, uint32_t index)
{
    AspRunResult result = AspRunResult_OK;

    result = AspAssert
        (engine, node != 0 && IsNodeType(AspDataGetType(node)));
    if (result != AspRunResult_OK)
        return result;

    if (AspDataGetType(node) == DataType_SetNode)
        AspDataSetSetNodeLeftIndex(node, index);
    else
    {
        AspDataEntry *linksNode = AspEntry
            (engine, AspDataGetTreeNodeLinksIndex(node));
        if (linksNode == 0 && index != 0)
        {
            linksNode = AspAllocEntry(engine, DataType_TreeLinksNode);
            if (linksNode == 0)
                return AspRunResult_OutOfDataMemory;
            AspDataSetTreeNodeLinksIndex(node, AspIndex(engine, linksNode));
        }
        if (linksNode != 0)
        {
            AspDataSetTreeLinksNodeLeftIndex(linksNode, index);
            PruneLinks(engine, node);
        }
    }

    return result;
}

static uint32_t GetLeftIndex(AspEngine *engine, AspDataEntry *node)
{
    AspRunResult assertResult = AspAssert
        (engine, node != 0 && IsNodeType(AspDataGetType(node)));
    if (assertResult != AspRunResult_OK)
        return 0;

    uint32_t leftIndex = 0;
    if (AspDataGetType(node) == DataType_SetNode)
        leftIndex = AspDataGetSetNodeLeftIndex(node);
    else
    {
        AspDataEntry *linksNode = AspEntry
            (engine, AspDataGetTreeNodeLinksIndex(node));
        if (linksNode != 0)
            leftIndex = AspDataGetTreeLinksNodeLeftIndex(linksNode);
    }

    return leftIndex;
}

static AspRunResult SetRightIndex
    (AspEngine *engine, AspDataEntry *node, uint32_t index)
{
    AspRunResult result = AspRunResult_OK;

    result = AspAssert
        (engine, node != 0 && IsNodeType(AspDataGetType(node)));
    if (result != AspRunResult_OK)
        return result;

    if (AspDataGetType(node) == DataType_SetNode)
        AspDataSetSetNodeRightIndex(node, index);
    else
    {
        AspDataEntry *linksNode = AspEntry
            (engine, AspDataGetTreeNodeLinksIndex(node));
        if (linksNode == 0 && index != 0)
        {
            linksNode = AspAllocEntry(engine, DataType_TreeLinksNode);
            if (linksNode == 0)
                return AspRunResult_OutOfDataMemory;
            AspDataSetTreeNodeLinksIndex(node, AspIndex(engine, linksNode));
        }
        if (linksNode != 0)
        {
            AspDataSetTreeLinksNodeRightIndex(linksNode, index);
            PruneLinks(engine, node);
        }
    }

    return result;
}

static uint32_t GetRightIndex(AspEngine *engine, AspDataEntry *node)
{
    AspRunResult assertResult = AspAssert
        (engine, node != 0 && IsNodeType(AspDataGetType(node)));
    if (assertResult != AspRunResult_OK)
        return 0;

    uint32_t rightIndex = 0;
    if (AspDataGetType(node) == DataType_SetNode)
        rightIndex = AspDataGetSetNodeRightIndex(node);
    else
    {
        AspDataEntry *linksNode = AspEntry
            (engine, AspDataGetTreeNodeLinksIndex(node));
        if (linksNode != 0)
            rightIndex = AspDataGetTreeLinksNodeRightIndex(linksNode);
    }

    return rightIndex;
}

static void PruneLinks(AspEngine *engine, AspDataEntry *node)
{
    AspRunResult assertResult = AspAssert
        (engine, node != 0 && IsNodeType(AspDataGetType(node)));
    if (assertResult != AspRunResult_OK)
        return;

    if (AspDataGetType(node) == DataType_SetNode)
        return;

    AspDataEntry *linksNode = AspEntry
        (engine, AspDataGetTreeNodeLinksIndex(node));
    if (linksNode == 0 ||
        AspDataGetTreeLinksNodeLeftIndex(linksNode) != 0 ||
        AspDataGetTreeLinksNodeRightIndex(linksNode) != 0)
        return;

    AspDataSetTreeNodeLinksIndex(node, 0);
    AspUnref(engine, linksNode);
}

static bool IsTreeType(DataType type)
{
    return
        type == DataType_Set ||
        type == DataType_Dictionary ||
        type == DataType_Namespace;
}

static bool IsNodeType(DataType type)
{
    return
        type == DataType_SetNode ||
        type == DataType_DictionaryNode ||
        type == DataType_NamespaceNode;
}

static AspRunResult NotFoundResult(AspDataEntry *tree)
{
    return AspDataGetType(tree) == DataType_Namespace ?
        AspRunResult_NameNotFound : AspRunResult_KeyNotFound;
}
