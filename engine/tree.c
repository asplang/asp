/*
 * Asp engine tree implementation.
 *
 * The red-black tree logic implemented here is based on the algorithms
 * described in Introduction to Algorithms, Thomas H. Cormen, et al., 3e.
 */

#include "tree.h"
#include "data.h"
#include "compare.h"

static AspRunResult Insert
    (AspEngine *, AspDataEntry *tree, AspDataEntry *node);
static AspDataEntry *FindNode
    (AspEngine *, AspDataEntry *tree, AspDataEntry *keyNode);
static AspDataEntry *GetLimitNode
    (AspEngine *, AspDataEntry *tree, AspDataEntry *node, bool right);
static AspRunResult Shift
    (AspEngine *, AspDataEntry *tree,
     AspDataEntry *node1, AspDataEntry *node2);
static AspRunResult Rotate
    (AspEngine *, AspDataEntry *tree, AspDataEntry *node, bool right);
static AspRunResult CompareKeys
    (AspEngine *, const AspDataEntry *tree,
     const AspDataEntry *leftNode, const AspDataEntry *rightNode,
     int *comparison);
static AspRunResult SetChildIndex
    (AspEngine *, AspDataEntry *node, bool right, uint32_t index);
static uint32_t GetChildIndex(AspEngine *, AspDataEntry *node, bool right);
static void PruneLinks(AspEngine *, AspDataEntry *node);
static bool IsTreeType(DataType type);
static bool IsNodeType(DataType type);
static AspRunResult NotFoundResult(AspDataEntry *tree);

#ifdef ASP_TEST
static bool IsRedBlack
    (AspEngine *, AspDataEntry *node, unsigned depth, unsigned *blackDepth);
static void Tally
    (AspEngine *, AspDataEntry *node, unsigned *tally);
#endif

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

    bool isImmutable;
    AspRunResult isImmutableResult = AspCheckIsImmutableObject
        (engine, key, &isImmutable);
    if (isImmutableResult != AspRunResult_OK)
    {
        result.result = isImmutableResult;
        return result;
    }
    if (!isImmutable)
    {
        result.result = AspRunResult_UnexpectedType;
        return result;
    }

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
    if (engine->runResult != AspRunResult_OK)
    {
        result.result = engine->runResult;
        return result;
    }
    if (foundNode != 0)
    {
        AspUnref(engine, result.node);
        if (engine->runResult != AspRunResult_OK)
        {
            result.result = engine->runResult;
            return result;
        }
        result.node = foundNode;
        result.key = AspValueEntry
            (engine, AspDataGetTreeNodeKeyIndex(foundNode));

        /* Replace the entry's value if applicable. */
        if (treeType != DataType_Set)
        {
            AspDataEntry *oldValue = AspValueEntry
                (engine, AspDataGetTreeNodeValueIndex(foundNode));
            if (AspIsObject(oldValue))
            {
                AspUnref(engine, oldValue);
                if (engine->runResult != AspRunResult_OK)
                {
                    result.result = engine->runResult;
                    return result;
                }
            }
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
    AspAssert
        (engine, symbol >= AspSignedWordMin && symbol <= AspSignedWordMax);
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

    AspDataEntry *node = FindNode(engine, tree, keyNode);
    if (engine->runResult != AspRunResult_OK)
        return engine->runResult;
    if (node == 0)
        return NotFoundResult(tree);

    /* Remove node from tree and determine whether rebalancing is required. */
    bool rebalance = AspDataGetTreeNodeIsBlack(node);
    uint32_t nodeIndex = AspIndex(engine, node);
    uint32_t parentIndex = AspDataGetTreeNodeParentIndex(node);
    uint32_t leftIndex = GetChildIndex(engine, node, false);
    uint32_t rightIndex = GetChildIndex(engine, node, true);
    AspDataEntry *fixNode = node;
    if (leftIndex == 0)
    {
        if (rightIndex != 0)
        {
            fixNode = AspEntry(engine, rightIndex);
            result = Shift(engine, tree, node, fixNode);
            if (result != AspRunResult_OK)
                return result;
        }
    }
    else if (rightIndex == 0)
    {
        fixNode = AspEntry(engine, leftIndex);
        result = Shift(engine, tree, node, fixNode);
        if (result != AspRunResult_OK)
            return result;
    }
    else
    {
        AspTreeResult nextResult = AspTreeNext(engine, tree, node, true);
        if (nextResult.result != AspRunResult_OK)
            return nextResult.result;
        AspDataEntry *nextNode = nextResult.node;
        result = AspAssert(engine, nextNode != 0);
        if (result != AspRunResult_OK)
            return result;
        uint32_t nextIndex = AspIndex(engine, nextNode);
        rebalance = AspDataGetTreeNodeIsBlack(nextNode);

        bool nodeIsBlack = AspDataGetTreeNodeIsBlack(node);
        AspDataEntry *nextRightNode = AspEntry
            (engine, GetChildIndex(engine, nextNode, true));
        if (nextRightNode != 0)
            fixNode = nextRightNode;

        if (AspDataGetTreeNodeParentIndex(nextNode) == AspIndex(engine, node))
        {
            AspDataSetTreeNodeParentIndex(fixNode, nextIndex);
            if (fixNode == node)
                SetChildIndex
                    (engine, nextNode, true, AspIndex(engine, fixNode));
        }
        else
        {
            result = Shift(engine, tree, nextNode, fixNode);
            if (result != AspRunResult_OK)
                return result;
            result = SetChildIndex(engine, nextNode, true, rightIndex);
            if (result != AspRunResult_OK)
                return result;
            AspDataEntry *rightNode = AspEntry
                (engine, GetChildIndex(engine, nextNode, true));
            AspDataSetTreeNodeParentIndex(rightNode, nextIndex);
        }

        if (parentIndex == 0)
            AspDataSetTreeRootIndex(tree, nextIndex);
        else
        {
            AspDataEntry *parentNode = AspEntry(engine, parentIndex);
            result = SetChildIndex
                (engine, parentNode,
                 nodeIndex == GetChildIndex(engine, parentNode, true),
                 nextIndex);
            if (result != AspRunResult_OK)
                return result;
        }
        AspDataSetTreeNodeParentIndex(nextNode, parentIndex);

        result = SetChildIndex(engine, nextNode, false, leftIndex);
        if (result != AspRunResult_OK)
            return result;
        AspDataEntry *leftNode = AspEntry(engine, leftIndex);
        AspDataSetTreeNodeParentIndex(leftNode, nextIndex);
        AspDataSetTreeNodeIsBlack(nextNode, nodeIsBlack);
        if (fixNode == node)
            AspDataSetTreeNodeIsBlack(fixNode, true);
    }

    uint8_t type = AspDataGetType(node);
    if (eraseKey && type != DataType_NamespaceNode)
    {
        AspUnref(engine, AspValueEntry
            (engine, AspDataGetTreeNodeKeyIndex(node)));
        if (engine->runResult != AspRunResult_OK)
            return engine->runResult;
        AspDataSetTreeNodeKeyIndex(node, 0);
    }
    if (eraseValue && type != DataType_SetNode)
    {
        AspDataEntry *value = AspValueEntry
            (engine, AspDataGetTreeNodeValueIndex(node));
        if (AspIsObject(value))
        {
            AspUnref(engine, value);
            if (engine->runResult != AspRunResult_OK)
                return engine->runResult;
        }
        AspDataSetTreeNodeValueIndex(node, 0);
    }
    if (type != DataType_SetNode)
    {
        uint32_t linksIndex = AspDataGetTreeNodeLinksIndex(node);
        if (linksIndex != 0)
        {
            AspUnref(engine, AspEntry(engine, linksIndex));
            if (engine->runResult != AspRunResult_OK)
                return engine->runResult;
            AspDataSetTreeNodeLinksIndex(node, 0);
        }
    }

    /* Rebalance the tree if required. */
    if (rebalance)
    {
        AspDataEntry *workNode = fixNode;
        AspDataEntry *rootNode = AspEntry
            (engine, AspDataGetTreeRootIndex(tree));
        while (workNode != rootNode && AspDataGetTreeNodeIsBlack(workNode))
        {
            uint32_t workIndex = AspIndex(engine, workNode);
            AspDataEntry *parentNode = AspEntry
                (engine, AspDataGetTreeNodeParentIndex(workNode));
            bool right =
                GetChildIndex(engine, parentNode, true) == workIndex;
            AspDataEntry *siblingNode = AspEntry
                (engine, GetChildIndex(engine, parentNode, !right));
            result = AspAssert(engine, siblingNode != 0);
            if (result != AspRunResult_OK)
                return result;
            if (!AspDataGetTreeNodeIsBlack(siblingNode))
            {
                AspDataSetTreeNodeIsBlack(siblingNode, true);
                AspDataSetTreeNodeIsBlack(parentNode, false);
                result = Rotate(engine, tree, parentNode, right);
                if (result != AspRunResult_OK)
                    return result;
                parentNode = AspEntry
                    (engine, AspDataGetTreeNodeParentIndex(workNode));
                siblingNode = AspEntry
                    (engine, GetChildIndex(engine, parentNode, !right));
                result = AspAssert(engine, siblingNode != 0);
                if (result != AspRunResult_OK)
                    return result;
            }

            AspDataEntry *nearNode = AspEntry
                (engine, GetChildIndex(engine, siblingNode, right));
            AspDataEntry *farNode = AspEntry
                (engine, GetChildIndex(engine, siblingNode, !right));
            if ((nearNode == 0 || AspDataGetTreeNodeIsBlack(nearNode)) &&
                (farNode == 0 || AspDataGetTreeNodeIsBlack(farNode)))
            {
                AspDataSetTreeNodeIsBlack(siblingNode, false);
                workNode = parentNode;
            }
            else
            {
                if (farNode == 0 || AspDataGetTreeNodeIsBlack(farNode))
                {
                    if (nearNode != 0)
                        AspDataSetTreeNodeIsBlack(nearNode, true);
                    AspDataSetTreeNodeIsBlack(siblingNode, false);
                    result = Rotate(engine, tree, siblingNode, !right);
                    if (result != AspRunResult_OK)
                        return result;
                    parentNode = AspEntry
                        (engine, AspDataGetTreeNodeParentIndex(workNode));
                    siblingNode = AspEntry
                        (engine, GetChildIndex(engine, parentNode, !right));
                    farNode = AspEntry
                        (engine, GetChildIndex(engine, siblingNode, !right));
                }

                AspDataSetTreeNodeIsBlack
                    (siblingNode, AspDataGetTreeNodeIsBlack(parentNode));
                AspDataSetTreeNodeIsBlack(parentNode, true);
                if (farNode != 0)
                    AspDataSetTreeNodeIsBlack(farNode, true);
                result = Rotate(engine, tree, parentNode, right);
                if (result != AspRunResult_OK)
                    return result;
                break;
            }
        }
        if (workNode)
            AspDataSetTreeNodeIsBlack(workNode, true);
    }
    if (fixNode == node)
    {
        uint32_t fixParentIndex = AspDataGetTreeNodeParentIndex(fixNode);
        if (fixParentIndex == 0)
            AspDataSetTreeRootIndex(tree, 0);
        else
        {
            AspDataEntry *fixParentNode = AspEntry(engine, fixParentIndex);
            result = SetChildIndex
                (engine, fixParentNode,
                 nodeIndex == GetChildIndex(engine, fixParentNode, true),
                 0);
        }
    }

    AspUnref(engine, node);
    if (engine->runResult != AspRunResult_OK)
        return engine->runResult;
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

    bool isImmutable;
    AspRunResult isImmutableResult = AspCheckIsImmutableObject
        (engine, key, &isImmutable);
    if (isImmutableResult != AspRunResult_OK)
    {
        result.result = isImmutableResult;
        return result;
    }
    if (!isImmutable)
    {
        result.result = AspRunResult_UnexpectedType;
        return result;
    }

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
    if (engine->runResult != AspRunResult_OK)
    {
        result.result = engine->runResult;
        return result;
    }
    if (result.node != 0 && treeType != DataType_Set)
        result.value = AspValueEntry
            (engine, AspDataGetTreeNodeValueIndex(result.node));
    AspUnref(engine, keyNode);
    if (engine->runResult != AspRunResult_OK)
    {
        result.result = engine->runResult;
        return result;
    }

    return result;
}

AspTreeResult AspFindSymbol
    (AspEngine *engine, AspDataEntry *tree, int32_t symbol)
{
    AspTreeResult result = {AspRunResult_OK, 0, 0, 0, false};

    AspAssert(engine, tree != 0);
    AspAssert(engine, AspDataGetType(tree) == DataType_Namespace);
    result.result = AspAssert
        (engine, symbol >= AspSignedWordMin && symbol <= AspSignedWordMax);
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
    if (engine->runResult != AspRunResult_OK)
    {
        result.result = engine->runResult;
        return result;
    }
    if (result.node != 0)
        result.value = AspValueEntry
            (engine, AspDataGetTreeNodeValueIndex(result.node));
    AspUnref(engine, keyNode);
    if (engine->runResult != AspRunResult_OK)
    {
        result.result = engine->runResult;
        return result;
    }

    return result;
}

AspTreeResult AspTreeNext
    (AspEngine *engine, AspDataEntry *tree, AspDataEntry *node, bool right)
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
        result.node = GetLimitNode(engine, tree, rootNode, !right);
    }
    else if (GetChildIndex(engine, result.node, right) != 0)
    {
        AspDataEntry *nextNode = AspEntry
            (engine, GetChildIndex(engine, result.node, right));
        result.node = GetLimitNode(engine, tree, nextNode, !right);
    }
    else
    {
        uint32_t parentIndex = AspDataGetTreeNodeParentIndex(node);
        result.node = AspEntry(engine, parentIndex);
        while (result.node != 0 &&
               AspIndex(engine, node) ==
               GetChildIndex(engine, result.node, right))
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
        int comparison;
        AspRunResult compareResult = CompareKeys
            (engine, tree, node, targetNode, &comparison);
        if (compareResult != AspRunResult_OK)
            return compareResult;
        uint32_t childIndex = GetChildIndex
            (engine, targetNode, comparison > 0);
        targetNode = AspEntry(engine, childIndex);
    }

    AspDataSetTreeNodeParentIndex(node, AspIndex(engine, parentNode));
    uint32_t nodeIndex = AspIndex(engine, node);
    if (parentNode == 0)
        AspDataSetTreeRootIndex(tree, nodeIndex);
    else
    {
        int comparison;
        AspRunResult compareResult = CompareKeys
            (engine, tree, node, parentNode, &comparison);
        result = SetChildIndex
            (engine, parentNode, comparison > 0,
             nodeIndex);
        if (result != AspRunResult_OK)
            return result;

        /* Rebalance the tree. */
        AspDataEntry *workNode = node;
        uint32_t workIndex = nodeIndex;
        while (parentNode != 0 && !AspDataGetTreeNodeIsBlack(parentNode))
        {
            uint32_t grandparentIndex = AspDataGetTreeNodeParentIndex
                (parentNode);
            AspDataEntry *grandparentNode = AspEntry(engine, grandparentIndex);
            result = AspAssert(engine, grandparentNode != 0);
            if (result != AspRunResult_OK)
                return result;

            uint32_t parentIndex = AspIndex(engine, parentNode);
            bool right =
                GetChildIndex(engine, grandparentNode, true) == parentIndex;
            AspDataEntry *uncleNode = AspEntry
                (engine, GetChildIndex(engine, grandparentNode, !right));
            if (uncleNode != 0 && !AspDataGetTreeNodeIsBlack(uncleNode))
            {
                AspDataSetTreeNodeIsBlack(parentNode, true);
                AspDataSetTreeNodeIsBlack(uncleNode, true);
                AspDataSetTreeNodeIsBlack(grandparentNode, false);
                workNode = grandparentNode;
                workIndex = grandparentIndex;
                parentNode = AspEntry
                    (engine, AspDataGetTreeNodeParentIndex(workNode));
            }
            else
            {
                if (workIndex == GetChildIndex(engine, parentNode, !right))
                {
                    workNode = parentNode;
                    workIndex = parentIndex;
                    result = Rotate(engine, tree, workNode, right);
                    if (result != AspRunResult_OK)
                        return result;
                    parentNode = AspEntry
                        (engine, AspDataGetTreeNodeParentIndex(workNode));
                    grandparentIndex = AspDataGetTreeNodeParentIndex
                        (parentNode);
                    grandparentNode = AspEntry(engine, grandparentIndex);
                }

                AspDataSetTreeNodeIsBlack(parentNode, true);
                AspDataSetTreeNodeIsBlack(grandparentNode, false);
                result = Rotate(engine, tree, grandparentNode, !right);
                if (result != AspRunResult_OK)
                    return result;
                parentNode = AspEntry
                    (engine, AspDataGetTreeNodeParentIndex(workNode));
            }
        }
    }
    AspDataSetTreeNodeIsBlack
        (AspEntry(engine, AspDataGetTreeRootIndex(tree)), true);

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
    while (node != 0)
    {
        int comparison;
        AspRunResult compareResult = CompareKeys
            (engine, tree, keyNode, node, &comparison);
        assertResult = AspAssert(engine, compareResult == AspRunResult_OK);
        if (assertResult != AspRunResult_OK)
            return 0;
        if (comparison == 0)
            break;
        AspDataEntry *nextNode = AspEntry
            (engine, GetChildIndex(engine, node, comparison > 0));
        assertResult = AspAssert(engine, nextNode != node);
        if (assertResult != AspRunResult_OK)
            return 0;
        node = nextNode;
    }

    return node;
}

static AspDataEntry *GetLimitNode
    (AspEngine *engine, AspDataEntry *tree, AspDataEntry *node, bool right)
{
    AspAssert
        (engine, tree != 0 && IsTreeType(AspDataGetType(tree)));
    AspRunResult assertResult = AspAssert
        (engine, node != 0 && IsNodeType(AspDataGetType(node)));
    if (assertResult != AspRunResult_OK)
        return 0;

    uint32_t childIndex;
    while ((childIndex = GetChildIndex(engine, node, right)) != 0)
        node = AspEntry(engine, childIndex);

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
    AspAssert
        (engine, node2 != 0 && IsNodeType(AspDataGetType(node2)));
    if (result != AspRunResult_OK)
        return 0;

    uint32_t node1ParentIndex = AspDataGetTreeNodeParentIndex(node1);
    if (node1ParentIndex == 0)
        AspDataSetTreeRootIndex(tree, AspIndex(engine, node2));
    else
    {
        AspDataEntry *parent1Node = AspEntry(engine, node1ParentIndex);
        uint32_t index2 = AspIndex(engine, node2);
        result = SetChildIndex
            (engine, parent1Node,
             AspIndex(engine, node1) ==
             GetChildIndex(engine, parent1Node, true),
             index2);
    }
    if (result != AspRunResult_OK)
        return result;

    AspDataSetTreeNodeParentIndex(node2, node1ParentIndex);

    return result;
}

static AspRunResult Rotate
    (AspEngine *engine, AspDataEntry *tree, AspDataEntry *node, bool right)
{
    AspRunResult result = AspRunResult_OK;

    AspAssert
        (engine, tree != 0 && IsTreeType(AspDataGetType(tree)));
    result = AspAssert
        (engine, node != 0 && IsNodeType(AspDataGetType(node)));
    if (result != AspRunResult_OK)
        return result;

    uint32_t nodeIndex = AspIndex(engine, node);
    uint32_t parentIndex = AspDataGetTreeNodeParentIndex(node);
    uint32_t siblingIndex = GetChildIndex(engine, node, !right);
    result = AspAssert(engine, siblingIndex != 0);
    if (result != AspRunResult_OK)
        return result;
    AspDataEntry *siblingNode = AspEntry(engine, siblingIndex);
    uint32_t nephewIndex = GetChildIndex(engine, siblingNode, right);

    result = SetChildIndex(engine, node, !right, nephewIndex);
    if (result != AspRunResult_OK)
        return result;
    if (nephewIndex != 0)
        AspDataSetTreeNodeParentIndex
            (AspEntry(engine, nephewIndex), nodeIndex);

    result = SetChildIndex(engine, siblingNode, right, nodeIndex);
    if (result != AspRunResult_OK)
        return result;
    AspDataSetTreeNodeParentIndex(node, siblingIndex);

    AspDataSetTreeNodeParentIndex(siblingNode, parentIndex);
    if (parentIndex != 0)
    {
        AspDataEntry *parentNode = AspEntry(engine, parentIndex);
        result = SetChildIndex
            (engine, parentNode,
             nodeIndex == GetChildIndex(engine, parentNode, true),
             siblingIndex);
    }
    else
        AspDataSetTreeRootIndex(tree, siblingIndex);

    return result;
}

static AspRunResult CompareKeys
    (AspEngine *engine, const AspDataEntry *tree,
     const AspDataEntry *leftNode, const AspDataEntry *rightNode,
     int *comparison)
{
    *comparison = 0;
    AspAssert
        (engine, tree != 0 && IsTreeType(AspDataGetType(tree)));
    AspAssert
        (engine, leftNode != 0 && IsNodeType(AspDataGetType(leftNode)));
    AspRunResult assertResult = AspAssert
        (engine, rightNode != 0 && IsNodeType(AspDataGetType(rightNode)));
    if (assertResult != AspRunResult_OK)
        return assertResult;

    /* For namespaces, compare the symbols. Otherwise, compare objects. */
    if (AspDataGetType(tree) == DataType_Namespace)
    {
        int32_t leftSymbol = AspDataGetNamespaceNodeSymbol(leftNode);
        int32_t rightSymbol = AspDataGetNamespaceNodeSymbol(rightNode);
        *comparison =
            leftSymbol == rightSymbol ? 0 :
            leftSymbol < rightSymbol ? -1 : 1;
        return AspRunResult_OK;
    }
    else
        return AspCompare
            (engine,
             AspValueEntry(engine, AspDataGetTreeNodeKeyIndex(leftNode)),
             AspValueEntry(engine, AspDataGetTreeNodeKeyIndex(rightNode)),
             AspCompareType_Key, comparison, 0);
}

static AspRunResult SetChildIndex
    (AspEngine *engine, AspDataEntry *node, bool right, uint32_t index)
{
    AspRunResult result = AspRunResult_OK;

    result = AspAssert
        (engine, node != 0 && IsNodeType(AspDataGetType(node)));
    if (result != AspRunResult_OK)
        return result;

    if (AspDataGetType(node) == DataType_SetNode)
    {
        if (!right)
            AspDataSetSetNodeLeftIndex(node, index);
        else
            AspDataSetSetNodeRightIndex(node, index);
    }
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
            if (!right)
                AspDataSetTreeLinksNodeLeftIndex(linksNode, index);
            else
                AspDataSetTreeLinksNodeRightIndex(linksNode, index);
            PruneLinks(engine, node);
            if (engine->runResult != AspRunResult_OK)
                return engine->runResult;
        }
    }

    return result;
}

static uint32_t GetChildIndex
    (AspEngine *engine, AspDataEntry *node, bool right)
{
    AspRunResult assertResult = AspAssert
        (engine, node != 0 && IsNodeType(AspDataGetType(node)));
    if (assertResult != AspRunResult_OK)
        return 0;

    uint32_t index = 0;
    if (AspDataGetType(node) == DataType_SetNode)
        index = !right ?
            AspDataGetSetNodeLeftIndex(node) :
            AspDataGetSetNodeRightIndex(node);
    else
    {
        AspDataEntry *linksNode = AspEntry
            (engine, AspDataGetTreeNodeLinksIndex(node));
        if (linksNode != 0)
            index = !right ?
                AspDataGetTreeLinksNodeLeftIndex(linksNode) :
                AspDataGetTreeLinksNodeRightIndex(linksNode);
    }

    return index;
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

#ifdef ASP_TEST

bool AspTreeIsRedBlack(AspEngine *engine, AspDataEntry *tree)
{
    uint32_t rootIndex = AspDataGetTreeRootIndex(tree);
    if (rootIndex == 0)
        return true;

    AspDataEntry *rootNode = AspEntry(engine, rootIndex);
    if (!AspDataGetTreeNodeIsBlack(rootNode))
        return false;

    unsigned blackDepth = 0;
    return IsRedBlack(engine, rootNode, 0, &blackDepth);
}

static bool IsRedBlack
    (AspEngine *engine, AspDataEntry *node,
     unsigned depth, unsigned *blackDepth)
{
    uint32_t leftIndex = GetChildIndex(engine, node, false);
    AspDataEntry *leftNode = AspEntry(engine, leftIndex);
    uint32_t rightIndex = GetChildIndex(engine, node, true);
    AspDataEntry *rightNode = AspEntry(engine, rightIndex);
    if (!AspDataGetTreeNodeIsBlack(node) &&
        leftIndex != 0 && rightIndex != 0 &&
        (!AspDataGetTreeNodeIsBlack(leftNode) ||
         !AspDataGetTreeNodeIsBlack(rightNode)))
        return false;

    if (AspDataGetTreeNodeIsBlack(node))
        depth++;

    if (leftIndex == 0 && rightIndex == 0)
    {
        if (*blackDepth == 0)
            *blackDepth = depth + 1;
        else if (*blackDepth != depth + 1)
            return false;
    }

    return
        (leftIndex == 0 || IsRedBlack(engine, leftNode, depth, blackDepth)) &&
        (rightIndex == 0 || IsRedBlack(engine, rightNode, depth, blackDepth));
}

unsigned AspTreeTally(AspEngine *engine, AspDataEntry *tree)
{
    uint32_t rootIndex = AspDataGetTreeRootIndex(tree);
    if (rootIndex == 0)
        return 0;
    unsigned tally = 0;
    Tally(engine, AspEntry(engine, rootIndex), &tally);
    return tally;
}

static void Tally(AspEngine *engine, AspDataEntry *node, unsigned *tally)
{
    uint32_t leftIndex = GetChildIndex(engine, node, false);
    if (leftIndex != 0)
        Tally(engine, AspEntry(engine, leftIndex), tally);

    (*tally)++;

    uint32_t rightIndex = GetChildIndex(engine, node, true);
    if (rightIndex != 0)
        Tally(engine, AspEntry(engine, rightIndex), tally);
}

#endif /* ASP_TEST */
