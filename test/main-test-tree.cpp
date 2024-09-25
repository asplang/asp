//
// Red-black tree testing main.
//

#include "asp.h"
#include "tree.h"
#include <vector>
#include <iostream>
#include <iomanip>
#include <memory>

using namespace std;

static void PrintTree
    (AspEngine *engine, const AspDataEntry *tree, ostream &);
static void PrintNode
    (AspEngine *, const AspDataEntry *node,
     int side, unsigned level, unsigned *depth, ostream &);

static const size_t DATA_ENTRY_COUNT = 2048;
static const size_t MAX_TREE_SIZE = 1017;
static const size_t SAMPLE_TREE_SIZE = 10;

int main(int argc, char **argv)
{
    // Determine byte size of data area.
    size_t dataEntrySize = AspDataEntrySize();
    size_t dataByteSize = DATA_ENTRY_COUNT * dataEntrySize;

    // Initialize the Asp engine.
    AspEngine engine;
    auto data = unique_ptr<char>(new char[dataByteSize]);
    AspRunResult initializeResult = AspInitialize
        (&engine,
         nullptr, 0, data.get(), dataByteSize,
         nullptr, nullptr);
    if (initializeResult != AspRunResult_OK)
    {
        auto oldFlags = cerr.flags();
        auto oldFill = cerr.fill();
        cerr
            << "Error 0x" << hex << uppercase << setfill('0')
            << setw(2) << initializeResult
            << " initializing Asp engine" << endl;
        cerr.flags(oldFlags);
        cerr.fill(oldFill);
        return 2;
    }

    auto set = AspNewSet(&engine);

    for (unsigned k = 1; k <= MAX_TREE_SIZE; k++)
    {
        cout << "Testing with " << k << " entries" << endl;

        // Insert entries.
        vector<AspDataEntry *> keys(k);
        for (unsigned i = 0; i < k; i++)
        {
            int j = (i + k / 2) % k;

            keys[j] = AspNewInteger(&engine, j);
            AspTreeResult insertResult = AspTreeInsert
                (&engine, set, keys[j], nullptr);
            if (insertResult.result != AspRunResult_OK)
            {
                PrintTree(&engine, set, cerr);
                auto oldFlags = cerr.flags();
                auto oldFill = cerr.fill();
                cerr
                    << "Insert result = 0x" << hex << uppercase << setfill('0')
                    << setw(2) << insertResult.result << endl;
                cerr.flags(oldFlags);
                cerr.fill(oldFill);
                return 1;
            }
            AspUnref(&engine, keys[j]);

            unsigned tally = AspTreeTally(&engine, set);
            int32_t count;
            AspCount(&engine, set, &count);
            if (tally != (unsigned)count)
            {
                PrintTree(&engine, set, cerr);
                cerr << "Dump:" << endl;
                #ifdef ASP_DEBUG
                AspDump(&engine, stderr);
                #endif
                cerr << "Tree corrupted after insert!" << endl;
                return 1;
            }
            if (!AspTreeIsRedBlack(&engine, set))
            {
                PrintTree(&engine, set, cerr);
                cerr << "Tree is not red-black after insert!" << endl;
                return 1;
            }
        }

        // Print out a sample tree.
        if (k == SAMPLE_TREE_SIZE)
        {
            cout << "\nSample tree:" << endl;
            PrintTree(&engine, set, cout);
            cout << endl;
        }

        // Erase entries.
        for (unsigned i = 0; i < k; i++)
        {
            // TODO: loop with all order permutations.
            int j = (i + k / 2 + 1) % k;

            auto iNode = AspTreeFind(&engine, set, keys[j]).node;
            if (iNode == nullptr)
            {
                cerr << "COULD NOT FIND KEY" << endl;
                break;
            }

            AspRunResult eraseResult = AspTreeEraseNode
                (&engine, set, iNode, true, true);
            if (eraseResult != AspRunResult_OK)
            {
                PrintTree(&engine, set, cerr);
                auto oldFlags = cerr.flags();
                auto oldFill = cerr.fill();
                cerr
                    << "Erase result = 0x" << hex << uppercase << setfill('0')
                    << setw(2) << eraseResult << endl;
                cerr.flags(oldFlags);
                cerr.fill(oldFill);
                return 1;
            }

            unsigned tally = AspTreeTally(&engine, set);
            int32_t count;
            AspCount(&engine, set, &count);
            if (tally != (unsigned)count)
            {
                PrintTree(&engine, set, cerr);
                cerr << "Dump:" << endl;
                #ifdef ASP_DEBUG
                AspDump(&engine, stdout);
                #endif
                cerr << "Tree corrupted after erase!" << endl;
                return 1;
            }
            if (!AspTreeIsRedBlack(&engine, set))
            {
                PrintTree(&engine, set, cerr);
                cerr << "Tree is not red-black after erase!" << endl;
                return 1;
            }
        }

        // Ensure tree is empty.
        int32_t count;
        AspCount(&engine, set, &count);
        if (count != 0)
        {
            PrintTree(&engine, set, cerr);
            cerr << "Dump:" << endl;
            #ifdef ASP_DEBUG
            AspDump(&engine, stderr);
            #endif
            cerr << "Final tree not empty!" << endl;
            return 1;
        }
    }

    cout
        << "\nLow free count: "
        << AspLowFreeCount(&engine)
        << " (max " << AspMaxDataSize(&engine) << ')' << endl;

    cout << "\nTest done." << endl;
    return 0;
}

static void PrintTree(AspEngine *engine, const AspDataEntry *tree, ostream &os)
{
    int32_t count;
    AspCount(engine, tree, &count);
    os << "Tree: count=" << count << '\n';
    uint32_t rootIndex = AspDataGetTreeRootIndex(tree);
    unsigned depth = 0;
    PrintNode(engine, AspEntry(engine, rootIndex), 0, 0, &depth, os);
    os << "End of tree: depth=" << depth << endl;
}

static void PrintNode
    (AspEngine *engine, const AspDataEntry *node, int side,
     unsigned level, unsigned *depth, ostream &os)
{
    if (level > *depth)
        *depth = level;

    string spaces(level, ' ');
    os
        << spaces
        << (side == -1 ? 'L' : side == 1 ? 'R' : 'T')
        << ": ";

    if (node == nullptr)
    {
        os << "~\n";
        return;
    }

    auto key = AspValueEntry
        (engine, AspDataGetTreeNodeKeyIndex(node));
    int32_t keyValue;
    if (AspIntegerValue(key, &keyValue))
        os << keyValue;
    else
        os << "?";

    os
        << ' '
        << (AspDataGetTreeNodeIsBlack(node) ? "(BLACK)" : "(RED)")
        << '\n';

    uint32_t leftIndex = 0;
    if (AspDataGetType(node) == DataType_SetNode)
        leftIndex = AspDataGetSetNodeLeftIndex(node);
    else
    {
        auto linksNode = AspEntry(engine, AspDataGetTreeNodeLinksIndex(node));
        if (linksNode != nullptr)
            leftIndex = AspDataGetTreeLinksNodeLeftIndex(linksNode);
    }
    auto leftNode = AspEntry(engine, leftIndex);
    PrintNode(engine, leftNode, -1, level + 1, depth, os);

    uint32_t rightIndex = 0;
    if (AspDataGetType(node) == DataType_SetNode)
        rightIndex = AspDataGetSetNodeRightIndex(node);
    else
    {
        auto linksNode = AspEntry(engine, AspDataGetTreeNodeLinksIndex(node));
        if (linksNode != nullptr)
            rightIndex = AspDataGetTreeLinksNodeRightIndex(linksNode);
    }
    auto rightNode = AspEntry(engine, rightIndex);
    PrintNode(engine, rightNode, +1, level + 1, depth, os);
}
