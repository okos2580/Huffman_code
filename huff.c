#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_CHAR 256

// 허프만 트리 노드 구조체 정의
struct HuffmanNode {
    char character;
    int frequency;
    struct HuffmanNode* left;
    struct HuffmanNode* right;
};

// 문자 빈도수를 저장하기 위한 구조체
struct CharFrequency {
    char character;
    int frequency;
};

// 기타 함수 선언
void calculateFrequencies(FILE* input, FILE* output);
struct HuffmanNode* buildHuffmanTree(struct CharFrequency* charFreq, int numChars);
void compressFile(FILE* input, FILE* output, struct HuffmanNode* huffmanTree);
void decompressFile(FILE* input, FILE* output, struct HuffmanNode* huffmanTree);
void destroyHuffmanTree(struct HuffmanNode* node);

int main() {
    // 1. 입력 파일 열기
    FILE* input = fopen("input.txt", "r");
    if (input == NULL) {
        printf("Error opening input file.\n");
        return 1;
    }

    // 2. 빈도수 계산 및 stats.txt에 저장
    FILE* stats = fopen("stats.txt", "w");
    if (stats == NULL) {
        printf("Error opening stats file.\n");
        fclose(input);
        return 1;
    }
    calculateFrequencies(input, stats);
    fclose(stats);

    // 3. stats.txt를 이용하여 Huffman 트리 생성
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

    // 4. Huffman 압축 수행
    FILE* compressed = fopen("output.huf", "wb");
    if (compressed == NULL) {
        printf("Error opening compressed file.\n");
        fclose(input);
        return 1;
    }
    rewind(input);
    compressFile(input, compressed, huffmanTree);
    fclose(compressed);

    // 5. 압축 해제 수행
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

    // 메모리 해제
    destroyHuffmanTree(huffmanTree);
    fclose(input);

    return 0;
}

// 입력 파일로부터 문자 빈도수를 계산하여 출력 파일에 저장하는 함수
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

// 허프만 트리 노드 생성 함수
struct HuffmanNode* createHuffmanNode(char character, int frequency) {
    struct HuffmanNode* node = (struct HuffmanNode*)malloc(sizeof(struct HuffmanNode));
    node->character = character;
    node->frequency = frequency;
    node->left = NULL;
    node->right = NULL;
    return node;
}

// 허프만 트리 노드 비교 함수 (빈도수 기준)
int compareNodes(const void* a, const void* b) {
    struct HuffmanNode** nodeA = (struct HuffmanNode**)a;
    struct HuffmanNode** nodeB = (struct HuffmanNode**)b;
    return (*nodeA)->frequency - (*nodeB)->frequency;
}

// 허프만 트리 구성 함수
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

// 허프만 코드 테이블 생성 함수
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

// 허프만 압축 수행 함수
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

// 허프만 트리를 이용하여 압축 해제 수행 함수
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

// 허프만 트리 메모리 해제 함수
void destroyHuffmanTree(struct HuffmanNode* node) {
    if (node != NULL) {
        destroyHuffmanTree(node->left);
        destroyHuffmanTree(node->right);
        free(node);
    }
}
