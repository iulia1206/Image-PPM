#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h> 
#include <inttypes.h>

//quadtree node structure
typedef struct QuadtreeNode
{
	unsigned char blue, green, red;
	uint32_t area;
	int32_t top_left, top_right;
	int32_t bottom_left, bottom_right;
} __attribute__((packed)) QuadtreeNode;

//pixel structure with 3 color channels(RGB)
typedef struct PPMPixel
{
	unsigned char red;
	unsigned char green;
	unsigned char blue;
} __attribute__((packed)) PPMPixel;

//pmm file strucure 
typedef struct PPM
{
	char type[3];
	int width;
	int height;
	int max_color;
	PPMPixel* pixel_matrix;
} __attribute__((packed)) PPM;

//reads file in PMM structure
int readPPM(char* in_file_name, PPM* out_ppm)
{
	FILE* ppm_file = fopen(in_file_name, "rb");

	//check if the file can be opened 
	if (ppm_file == NULL)
	{
		return -1;
	}

	//reads type
	fscanf(ppm_file, "%s", out_ppm->type);
	out_ppm->type[2] = '\0';
	
	//make sure we have a P6 file
	if (strcmp(out_ppm->type, "P6") != 0)
	{
		return -1;
	}

	//read width and height and the maximum color
	fscanf(ppm_file, "%d", &(out_ppm->width));
	fscanf(ppm_file, "%d", &(out_ppm->height));
	fscanf(ppm_file, "%d", &(out_ppm->max_color));

	//jump over '\n'
	fseek(ppm_file, 1, SEEK_CUR);

	//allocate pixel 'matrix'
	out_ppm->pixel_matrix = (PPMPixel*)malloc(sizeof(PPMPixel) * (out_ppm->height) * (out_ppm->width));

	//read pixel matrix
	fread(out_ppm->pixel_matrix, sizeof(PPMPixel), (out_ppm->height) * (out_ppm->width), ppm_file);

	fclose(ppm_file);

	return 0;
}

//writes PPM structure into a file
int writePPM(char* in_file_name, PPM* ppm)
{
	FILE* ppm_file = fopen(in_file_name, "wb");

	if (ppm_file == NULL)
	{
		return -1;
	}

	//write type(should be P6)
	fprintf(ppm_file, "%s\n", ppm->type);

	//write width and heigh and the maximum color 
	fprintf(ppm_file, "%d %d", ppm->width, ppm->height);
	fprintf(ppm_file, "\n%d\n", ppm->max_color);

	//write pixel matrix
	fwrite(ppm->pixel_matrix, sizeof(PPMPixel), (ppm->height) * (ppm->width), ppm_file);

	fclose(ppm_file);

	return 0;
}

//free memory allocated for pixel matrix
void releasePPM(PPM* ppm)
{
	free(ppm->pixel_matrix);
}

//write compressed PMM file
int writeCompressedPPM(char* fileName, QuadtreeNode* vector, uint32_t nodeCount, uint32_t terminalNodeCount)
{
	FILE* compressed_ppm_file = fopen(fileName, "wb");

	if (compressed_ppm_file == NULL)
	{
		return -1;
	}

	fwrite(&terminalNodeCount, sizeof(uint32_t), 1, compressed_ppm_file);
	fwrite(&nodeCount, sizeof(uint32_t), 1, compressed_ppm_file);
	fwrite(vector, sizeof(QuadtreeNode), nodeCount, compressed_ppm_file);

	fclose(compressed_ppm_file);

	return 0;
}

//read compressed PMM file
int readCompressedPPM(char* fileName, QuadtreeNode** vector, uint32_t* nodeCount, uint32_t* terminalNodeCount)
{
	FILE* compressed_ppm_file = fopen(fileName, "rb");

	if (compressed_ppm_file == NULL)
	{
		return -1;
	}

	fread(terminalNodeCount, sizeof(uint32_t), 1, compressed_ppm_file);
	fread(nodeCount, sizeof(uint32_t), 1, compressed_ppm_file);

	*vector = (QuadtreeNode*)malloc(sizeof(QuadtreeNode) * (*nodeCount));

	fread(*vector, sizeof(QuadtreeNode), *nodeCount, compressed_ppm_file);

	fclose(compressed_ppm_file);

	return 0;
}

//calculate mean like in the formula 
unsigned int computePPMBlockSimilarityScore(PPM* ppm, int x, int y, int size, unsigned char* red, unsigned char* green, unsigned char* blue)
{
	unsigned int m_red = 0, m_green = 0, m_blue = 0;
	unsigned int mean = 0;
	int i, j;

	//calculate medium value for each color 
	for (i = x; i < x + size; i++)
	{
		for (j = y; j < y + size; j++)
		{
			m_red += (ppm->pixel_matrix + j * ppm->width + i)->red;
			m_green += (ppm->pixel_matrix + j * ppm->width + i)->green;
			m_blue += (ppm->pixel_matrix + j * ppm->width + i)->blue;
		}
	}

	m_red = m_red / (size * size);
	m_green = m_green / (size * size);
	m_blue = m_blue / (size * size);

	//calculate mean(similarity score)
	for (i = x; i < x + size; i++)
	{
		for (j = y; j < y + size; j++)
		{
			mean += (m_red - (ppm->pixel_matrix + j * ppm->width + i)->red) * (m_red - (ppm->pixel_matrix + j * ppm->width + i)->red);
			mean += (m_green - (ppm->pixel_matrix + j * ppm->width + i)->green) * (m_green - (ppm->pixel_matrix + j * ppm->width + i)->green);
			mean += (m_blue - (ppm->pixel_matrix + j * ppm->width + i)->blue) * (m_blue - (ppm->pixel_matrix + j * ppm->width + i)->blue);
		}
	}
	mean = mean / (3 * size * size);

	*red = (unsigned char)m_red;
	*green = (unsigned char)m_green;
	*blue = (unsigned char)m_blue;

	return mean;
}

//build recursively the quadtree node for the PPM block(sqare) specified through x,y,size(top left corner and sqare side)
void ppmBlockToQuadTreeNode(PPM* ppm, int x, int y, int size, QuadtreeNode** vector, uint32_t* nodeCount, uint32_t* terminalNodeCount, int32_t parentIndex, unsigned int treshold, int blocType)
{
	unsigned char red = 0, green = 0, blue = 0;

	int32_t nodeIndex = *nodeCount;

	*vector = (QuadtreeNode*)realloc(*vector, sizeof(QuadtreeNode) * (*nodeCount + 1));
	*nodeCount = *nodeCount + 1;

	//this should be after realloc to avoid any memory issues due to block address change
	QuadtreeNode* parentNode = (*vector) + parentIndex;
	QuadtreeNode* node = (*vector) + nodeIndex;

	node->area = size * size; 
	node->top_left = -1;
	node->top_right = -1;
	node->bottom_left = -1;
	node->bottom_right = -1;

	if (blocType == 1)
	{
		//set top left index in vector
		parentNode->top_left = nodeIndex;
	}
	else if (blocType == 2)
	{
		//set top right index in vector
		parentNode->top_right = nodeIndex;
	}
	else if (blocType == 3)
	{
		//set bottom right index in vector
		parentNode->bottom_right = nodeIndex;
	}
	else if (blocType == 4)
	{
		//set bottom left index in vector
		parentNode->bottom_left = nodeIndex;
	}


	unsigned int mean = computePPMBlockSimilarityScore(ppm, x, y, size, &red, &green, &blue);

	//store the median color
	node->red = red;
	node->green = green;
	node->blue = blue;

	if (mean > treshold)
	{
		//top left child
		ppmBlockToQuadTreeNode(ppm, x, y, size / 2, vector, nodeCount, terminalNodeCount, nodeIndex, treshold, 1);

		//top right child
		ppmBlockToQuadTreeNode(ppm, x + size / 2, y, size / 2, vector, nodeCount, terminalNodeCount, nodeIndex, treshold, 2);


		//bottom right child
		ppmBlockToQuadTreeNode(ppm, x + size / 2, y + size / 2, size / 2, vector, nodeCount, terminalNodeCount, nodeIndex, treshold, 3);

		//bottom left child
		ppmBlockToQuadTreeNode(ppm, x, y + size / 2, size / 2, vector, nodeCount, terminalNodeCount, nodeIndex, treshold, 4);

	}
	else
	{
		//we found another terminal node
		*terminalNodeCount = *terminalNodeCount + 1;
	}
	parentNode = NULL;
	node = NULL;
}

//create the entire quadree from ppm structure
int ppmToQuadTree(PPM* ppm, QuadtreeNode** vector, uint32_t* nodeCount, uint32_t* terminalNodeCount, unsigned int treshold)
{
	unsigned char red = 0, green = 0, blue = 0;

	if (*vector != NULL)
		return -1;

	// create root node at index 0
	*vector = (QuadtreeNode*)malloc(sizeof(QuadtreeNode));

	(*vector)->area = ppm->width * ppm->height;
	(*vector)->top_left = -1;
	(*vector)->top_right = -1;
	(*vector)->bottom_left = -1;
	(*vector)->bottom_right = -1;
	*nodeCount = 1;

	unsigned int mean = computePPMBlockSimilarityScore(ppm, 0, 0, ppm->width, &red, &green, &blue);

	// Store the medium color
	(*vector)->red = red;
	(*vector)->green = green;
	(*vector)->blue = blue;

	if (mean > treshold)
	{
		// top left child
		ppmBlockToQuadTreeNode(ppm, 0, 0, ppm->width / 2, vector, nodeCount, terminalNodeCount, 0, treshold, 1);

		// top right child
		ppmBlockToQuadTreeNode(ppm, 0 + ppm->width / 2, 0, ppm->width / 2, vector, nodeCount, terminalNodeCount, 0, treshold, 2);

		// bottom right child
		ppmBlockToQuadTreeNode(ppm, 0 + ppm->width / 2, 0 + ppm->width / 2, ppm->width / 2, vector, nodeCount, terminalNodeCount, 0, treshold, 3);

		// bottom left child
		ppmBlockToQuadTreeNode(ppm, 0, 0 + ppm->width / 2, ppm->width / 2, vector, nodeCount, terminalNodeCount, 0, treshold, 4);

	}
	else
	{
		// We have only one node for all image
		*terminalNodeCount = 1;
	}

	return 0;
}

//free quadtree
void releaseQuadTree(QuadtreeNode** vector)
{
	free(*vector);
	*vector = NULL;
}

//create ppm structure from quadtree node
void quadTreeNodeToPPMBlock(QuadtreeNode* vector, uint32_t nodeCount, int32_t nodeIndex, int x, int y, PPM* ppm)
{
	QuadtreeNode* node = vector + nodeIndex;
	int size = (int)round(sqrt(node->area));
	// Check if we have a terminal node
	if (node->top_left == -1)
	{
		//calculate the indexes in the pixel matrix for node block
		for (int i = x; i < x + size; i++)
		{
			for (int j = y; j < y + size; j++)
			{

				//compute the index and set the pixel
				int index = j * ppm->width + i;

				(ppm->pixel_matrix + index)->red = node->red;
				(ppm->pixel_matrix + index)->green = node->green;
				(ppm->pixel_matrix + index)->blue = node->blue;
			}
		}
	}
	else
	{
		//process children

		//top left
		quadTreeNodeToPPMBlock(vector, nodeCount, node->top_left, x, y, ppm);

		//top right
		quadTreeNodeToPPMBlock(vector, nodeCount, node->top_right, x + size / 2, y, ppm);

		//bottom right
		quadTreeNodeToPPMBlock(vector, nodeCount, node->bottom_right, x + size / 2, y + size / 2, ppm);

		//bottom left
		quadTreeNodeToPPMBlock(vector, nodeCount, node->bottom_left, x, y + size / 2, ppm);
	}
}

//generate the entire ppm structure form the quadtree
void quadTreeToPPM(QuadtreeNode* vector, uint32_t nodeCount, PPM* ppm)
{
	ppm->width = (int)round(sqrt(vector->area));
	ppm->height = ppm->width;
	ppm->max_color = 255;
	strcpy(ppm->type, "P6");
	ppm->pixel_matrix = (PPMPixel*)malloc(sizeof(PPMPixel) * (ppm->height) * (ppm->width));

	//recursively build the pixel matrix
	quadTreeNodeToPPMBlock(vector, nodeCount, 0, 0, 0, ppm);
}

//generates recursively the horizontal mirror of a quadtree node/ppm block
void mirrorQuadTreeNodeHorizontal(QuadtreeNode* vector, int32_t nodeIndex)
{
	QuadtreeNode* node = (vector + nodeIndex);

	if (node->top_left == -1)
		return;

	mirrorQuadTreeNodeHorizontal(vector, node->top_left);
	mirrorQuadTreeNodeHorizontal(vector, node->top_right);
	mirrorQuadTreeNodeHorizontal(vector, node->bottom_left);
	mirrorQuadTreeNodeHorizontal(vector, node->bottom_right);

	//exchange top_left with top right and bottom_left with bottom right
	int32_t tmp = node->top_left;
	node->top_left = node->top_right;
	node->top_right = tmp;

	tmp = node->bottom_left;
	node->bottom_left = node->bottom_right;
	node->bottom_right = tmp;
}

//generates the horizontal mirror of the quadtree 
void mirrorQuadTreeHorizontal(QuadtreeNode* vector)
{
	mirrorQuadTreeNodeHorizontal(vector, 0);
}

//generates recursively the vertical mirror of a quadtree node/ppm block
void mirrorQuadTreeNodeVertical(QuadtreeNode* vector, int32_t nodeIndex)
{
	QuadtreeNode* node = (vector + nodeIndex);

	if (node->top_left == -1)
		return;

	mirrorQuadTreeNodeVertical(vector, node->top_left);
	mirrorQuadTreeNodeVertical(vector, node->top_right);
	mirrorQuadTreeNodeVertical(vector, node->bottom_right);
	mirrorQuadTreeNodeVertical(vector, node->bottom_left);
	
	//exchange top_left with bottom left and top_right with bottom right
	int32_t tmp = node->top_left;
	node->top_left = node->bottom_left;
	node->bottom_left = tmp;

	tmp = node->top_right;
	node->top_right = node->bottom_right;
	node->bottom_right = tmp;
}

//generates the vertical mirror of the quadtree 
void mirrorQuadTreeVertical(QuadtreeNode* vector)
{
	mirrorQuadTreeNodeVertical(vector, 0);
}

//create compressed file from ppm file
void compressPPMFile(char* ppmFile, char* compressedFile, int factor)
{
	PPM ppm;
	QuadtreeNode* vector = NULL;
	uint32_t nodeCount = 0;
	uint32_t terminalNodeCount = 0;

	readPPM(ppmFile, &ppm);
	ppmToQuadTree(&ppm, &vector, &nodeCount, &terminalNodeCount, factor);
	writeCompressedPPM(compressedFile, vector, nodeCount, terminalNodeCount);

	releasePPM(&ppm);
	releaseQuadTree(&vector);
}

//create decompressed file from ppm file
void decompressPPMFile(char* compressedFile, char* ppmFile)
{
	PPM ppm;
	QuadtreeNode* vector = NULL;
	uint32_t nodeCount = 0;
	uint32_t terminalNodeCount = 0;

	readCompressedPPM(compressedFile, &vector, &nodeCount, &terminalNodeCount);
	quadTreeToPPM(vector, nodeCount, &ppm);
	writePPM(ppmFile, &ppm);

	releasePPM(&ppm);
	releaseQuadTree(&vector);
}

//create horizontal mirror file from ppm file
void mirrorHorizontalPPMFile(char* ppmFile, char* ppmMirrorFile)
{
	PPM ppm;
	PPM ppmMirror;
	QuadtreeNode* vector = NULL;
	uint32_t nodeCount = 0;
	uint32_t terminalNodeCount = 0;

	readPPM(ppmFile, &ppm);
	ppmToQuadTree(&ppm, &vector, &nodeCount, &terminalNodeCount, 0);
	mirrorQuadTreeHorizontal(vector);
	quadTreeToPPM(vector, nodeCount, &ppmMirror);
	writePPM(ppmMirrorFile, &ppmMirror);

	releasePPM(&ppm);
	releasePPM(&ppmMirror);
	releaseQuadTree(&vector);

}

//create vertical mirror file from ppm file
void mirrorVerticalPPMFile(char* ppmFile, char* ppmMirrorFile)
{
	PPM ppm;
	PPM ppmMirror;
	QuadtreeNode* vector = NULL;
	uint32_t nodeCount = 0;
	uint32_t terminalNodeCount = 0;

	readPPM(ppmFile, &ppm);
	ppmToQuadTree(&ppm, &vector, &nodeCount, &terminalNodeCount, 0);
	mirrorQuadTreeVertical(vector);
	quadTreeToPPM(vector, nodeCount, &ppmMirror);
	writePPM(ppmMirrorFile, &ppmMirror);

	releasePPM(&ppm);
	releasePPM(&ppmMirror);
	releaseQuadTree(&vector);

}

int main(int argc, char* argv[])
{
	
	if (argc < 4 || argc > 6)
	{
		return -1;
	}

	if (argc == 4)
	{
		
		//we expect a decompress option -d
		if (strcmp(argv[1], "-d") != 0)
		{
			return -1;
		}
		decompressPPMFile(argv[2], argv[3]);
		return 0;
	}
	else 
		if (argc == 5)
		{

			if (strcmp(argv[1], "-c") == 0)
			{
				//compress
				int factor = atoi(argv[2]);
				compressPPMFile(argv[3], argv[4], factor);
				return 0;
			}
		}
		else
		{
			
			if (strcmp(argv[1], "-m") == 0 && strcmp(argv[2], "h") == 0)
			{
				//mirror horizontal		
				int factor = atoi(argv[3]);
				compressPPMFile(argv[4], argv[5], factor);
				decompressPPMFile(argv[5], argv[4]);
				mirrorHorizontalPPMFile(argv[4], argv[5]);
				return 0;
			}

			if (strcmp(argv[1], "-m") == 0 && strcmp(argv[2], "v") == 0)
			{
				//mirror vertical
				int factor = atoi(argv[3]);
				compressPPMFile(argv[4], argv[5], factor);
				decompressPPMFile(argv[5], argv[4]);			
				mirrorVerticalPPMFile(argv[4], argv[5]);
				return 0;
			}
		}

	//argument mismatch
	return -1;
}
