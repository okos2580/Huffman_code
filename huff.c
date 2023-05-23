#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_CHAR 256

// ������ Ʈ�� ��� ����ü ����
struct HuffmanNode {
    char character;
    int frequency;
    struct HuffmanNode* left;
    struct HuffmanNode* right;
};

// ���� �󵵼��� �����ϱ� ���� ����ü
struct CharFrequency {
    char character;
    int frequency;
};

// ��Ÿ �Լ� ����
void calculateFrequencies(FILE* input, FILE* output);
struct HuffmanNode* buildHuffmanTree(struct CharFrequency* charFreq, int numChars);
void compressFile(FILE* input, FILE* output, struct HuffmanNode* huffmanTree);
void decompressFile(FILE* input, FILE* output, struct HuffmanNode* huffmanTree);
void destroyHuffmanTree(struct HuffmanNode* node);

int main() {
    // 1. �Է� ���� ����
    FILE* input = fopen("input.txt", "r");
    if (input == NULL) {
        printf("Error opening input file.\n");
        return 1;
    }

    // 2. �󵵼� ��� �� stats.txt�� ����
    FILE* stats = fopen("stats.txt", "w");
    if (stats == NULL) {
        printf("Error opening stats file.\n");
        fclose(input);
        return 1;
    }
    calculateFrequencies(input, stats);
    fclose(stats);

    // 3. stats.txt�� �̿��Ͽ� Huffman Ʈ�� ����
    stats = fopen("stats.txt", "r");
    if (stats == NULL) {
        printf("Error opening stats file.\n");
        fclose(input);
        return 1;
    }
    struct CharFrequency charFreq[MAX_CHAR];
    int numChars = 0;
    char line[256];
    while (fgets(line, sizeof(line), stats) != NULL) {
        sscanf(line, "%c\t%d\n", &(charFreq[numChars].character), &(charFreq[numChars].frequency));
        numChars++;
    }
    struct HuffmanNode* huffmanTree = buildHuffmanTree(charFreq, numChars);
    fclose(stats);

    // 4. Huffman ���� ����
    FILE* compressed = fopen("output.huf", "wb");
    if (compressed == NULL) {
        printf("Error opening compressed file.\n");
        fclose(input);
        return 1;
    }
    rewind(input);
    compressFile(input, compressed, huffmanTree);
    fclose(compressed);

    // 5. ���� ���� ����
    compressed = fopen("output.huf", "rb");
    if (compressed == NULL) {
        printf("Error opening compressed file.\n");
        fclose(input);
        return 1;
    }
    FILE* decompressed = fopen("output.txt", "w");
    if (decompressed == NULL) {
        printf("Error opening decompressed file.\n");
        fclose(input);
        fclose(compressed);
        return 1;
    }
    decompressFile(compressed, decompressed, huffmanTree);
    fclose(decompressed);
    fclose(compressed);

    // �޸� ����
    destroyHuffmanTree(huffmanTree);
    fclose(input);

    return 0;
}

// �Է� ���Ϸκ��� ���� �󵵼��� ����Ͽ� ��� ���Ͽ� �����ϴ� �Լ�
void calculateFrequencies(FILE* input, FILE* output) {
    int charCount[MAX_CHAR] = { 0 };
    int c;

    while ((c = fgetc(input)) != EOF) {
        if (c >= 0 && c < MAX_CHAR) {
            charCount[c]++;
        }
    }

    for (int i = 0; i < MAX_CHAR; i++) {
        if (charCount[i] > 0) {
            fprintf(output, "%c\t%d\n", i, charCount[i]);
        }
    }
}

// ������ Ʈ�� ��� ���� �Լ�
struct HuffmanNode* createHuffmanNode(char character, int frequency) {
    struct HuffmanNode* node = (struct HuffmanNode*)malloc(sizeof(struct HuffmanNode));
    node->character = character;
    node->frequency = frequency;
    node->left = NULL;
    node->right = NULL;
    return node;
}

// ������ Ʈ�� ��� �� �Լ� (�󵵼� ����)
int compareNodes(const void* a, const void* b) {
    struct HuffmanNode** nodeA = (struct HuffmanNode**)a;
    struct HuffmanNode** nodeB = (struct HuffmanNode**)b;
    return (*nodeA)->frequency - (*nodeB)->frequency;
}

// ������ Ʈ�� ���� �Լ�
struct HuffmanNode* buildHuffmanTree(struct CharFrequency* charFreq, int numChars) {
    struct HuffmanNode** nodes = (struct HuffmanNode**)malloc(numChars * sizeof(struct HuffmanNode*));

    for (int i = 0; i < numChars; i++) {
        nodes[i] = createHuffmanNode(charFreq[i].character, charFreq[i].frequency);
    }

    while (numChars > 1) {
        qsort(nodes, numChars, sizeof(struct HuffmanNode*), compareNodes);

        struct HuffmanNode* left = nodes[0];
        struct HuffmanNode* right = nodes[1];
        struct HuffmanNode* parent = createHuffmanNode('$', left->frequency + right->frequency);
        parent->left = left;
        parent->right = right;

        nodes[0] = parent;
        nodes[1] = nodes[numChars - 1];

        numChars--;
    }

    struct HuffmanNode* root = nodes[0];
    free(nodes);

    return root;
}

// ������ �ڵ� ���̺� ���� �Լ�
void buildHuffmanCode(struct HuffmanNode* node, char* code, int depth, char** huffmanCode) {
    if (node->left == NULL && node->right == NULL) {
        code[depth] = '\0';
        huffmanCode[(int)node->character] = _strdup(code);
    }
    else {
        code[depth] = '0';
        buildHuffmanCode(node->left, code, depth + 1, huffmanCode);

        code[depth] = '1';
        buildHuffmanCode(node->right, code, depth + 1, huffmanCode);
    }
}

// ������ ���� ���� �Լ�
void compressFile(FILE* input, FILE* output, struct HuffmanNode* huffmanTree) {
    char* huffmanCode[MAX_CHAR] = { NULL };
    char code[MAX_CHAR] = { 0 };

    buildHuffmanCode(huffmanTree, code, 0, huffmanCode);

    int c;
    unsigned char buffer = 0;
    int bitCount = 0;

    while ((c = fgetc(input)) != EOF) {
        if (c >= 0 && c < MAX_CHAR) {
            char* huffCode = huffmanCode[c];
            for (int i = 0; i < strlen(huffCode); i++) {
                buffer = buffer << 1;
                if (huffCode[i] == '1') {
                    buffer = buffer | 1;
                }
                bitCount++;
                if (bitCount == 8) {
                    fputc(buffer, output);
                    buffer = 0;
                    bitCount = 0;
                }
            }
        }
    }

    if (bitCount > 0) {
        buffer = buffer << (8 - bitCount);
        fputc(buffer, output);
    }

    for (int i = 0; i < MAX_CHAR; i++) {
        free(huffmanCode[i]);
    }
}

// ������ Ʈ���� �̿��Ͽ� ���� ���� ���� �Լ�
void decompressFile(FILE* input, FILE* output, struct HuffmanNode* huffmanTree) {
    struct HuffmanNode* currentNode = huffmanTree;

    int c;
    while ((c = fgetc(input)) != EOF) {
        for (int i = 7; i >= 0; i--) {
            int bit = (c >> i) & 1;

            if (bit == 0) {
                currentNode = currentNode->left;
            }
            else {
                currentNode = currentNode->right;
            }

            if (currentNode->left == NULL && currentNode->right == NULL) {
                fputc(currentNode->character, output);
                currentNode = huffmanTree;
            }
        }
    }
}

// ������ Ʈ�� �޸� ���� �Լ�
void destroyHuffmanTree(struct HuffmanNode* node) {
    if (node != NULL) {
        destroyHuffmanTree(node->left);
        destroyHuffmanTree(node->right);
        free(node);
    }
}
