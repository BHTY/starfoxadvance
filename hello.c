#include <stdlib.h>
#include <string.h>
#include <math.h>
#define RGB16(r,g,b)  ((r)+(g<<5)+(b<<10)) 
#define SHOW_BACK 0x10;

#define BUTTON_A (1 << 0)
#define BUTTON_B (1 << 1)
#define BUTTON_SELECT (1 << 2)
#define BUTTON_START (1 << 3)
#define BUTTON_RIGHT (1 << 4)
#define BUTTON_LEFT (1 << 5)
#define BUTTON_UP (1 << 6)
#define BUTTON_DOWN (1 << 7)
#define BUTTON_R (1 << 8)
#define BUTTON_L (1 << 9)
#define sgn(x) ((x<0)?-1:((x>0)?1:0))
#define PI 3.14159

int state = 0;

volatile unsigned short* buttons = (volatile unsigned short*) 0x04000130;

unsigned char button_pressed(unsigned short button) {
    /* and the button register with the button constant we want */
    unsigned short pressed = *buttons & button;

    /* if this value is zero, then it's not pressed */
    if (pressed == 0) {
        return 1;
    } else {
        return 0;
    }
}

typedef struct Vector{
	float x;
	float y;
	float z;
} Vector;

typedef struct Triangle{
	Vector vert1;
	Vector vert2;
	Vector vert3;
	unsigned short shade;
} Triangle;

typedef struct Entity{
	Vector pos;
	Vector rot;
	Triangle* polygons;
	int numTris;
} Entity;

unsigned short* frontbuffer = (unsigned short*)0x6000000; 
unsigned short* backbuffer = (unsigned short*)0x600A000; 

float* matmul(float matrix[3][3], float* vector, float* needFloats) {
	float* newFloats = (float*)malloc(3 * sizeof(float));
	//memset(newFloats, 0, 3 * sizeof(float));
	int i, o;

	for (i = 0; i < 3; i++) {
		float temp = 0.0;
		for (o = 0; o < 3; o++) {
			temp += (matrix[i][o] * vector[o]);
		}
		newFloats[i] = temp; //should change the vector instead
		//vector[i] = temp;
	}

	memcpy(needFloats, newFloats, 3 * sizeof(float));
	free(newFloats);

	return needFloats;

}

unsigned short* pageFlip(unsigned short* buffer){
	if(buffer == frontbuffer){
		*(unsigned long*)0x4000000 &= ~SHOW_BACK;
		return backbuffer;
	}
	else{
		*(unsigned long*)0x4000000 |= SHOW_BACK;
		return frontbuffer;
	}
}

void writePx(unsigned char x, unsigned char y, unsigned short color, unsigned short* buffer){ //take advantage of calling convention to write *fast* transfer in asm
	buffer[x+(y<<7)+(y<<5)] = color; //160x128
}

void drawLine(unsigned short x1, unsigned short y1, unsigned short x2, unsigned short y2, unsigned short color, unsigned short* buffer);

void drawVerticalLine(unsigned short x1, unsigned short y1, unsigned short x2, unsigned short y2, unsigned short color, unsigned short* buffer){
	int y;
	
	for(y=y1; y<=y2; y++){
		writePx(x1, y, color, buffer);
	}
}

void drawLine(unsigned short x1, unsigned short y1, unsigned short x2, unsigned short y2, unsigned short color, unsigned short* buffer){
	/*float slope = (y2-y1)/(x2-x1);
	int x;
	
	for(x = x1; x<=x2; x++){
		writePx(x, slope*(x-x1) + y1, color);
	}*/
	
	if(x2 > x1){
		int dx = (x2 - x1);
		int dy = y2 - y1;
		int x;

		for(x=x1; x<=x2; x++){
    		int y = y1 + dy * (x - x1) / dx;
    		writePx(x, y, color, buffer);
		}
	}	
	else if(x2 < x1){
		drawLine(x2, y2, x1, y1, color, buffer);
	}
	else{
		drawVerticalLine(x1, y1, x2, y2, color, buffer);
	}
}

void drawHorizontalLine(void);

void blitmap(unsigned char x, unsigned char y, unsigned char xSize, unsigned char ySize, unsigned short* bitmap, unsigned short* buffer){
	int i, o;
	
	for(i=0; i<ySize; i++){
		
		if(i+y < 128){
			for(o=0; o<xSize; o++){
				short pixel = bitmap[o+i*xSize];
				if(pixel && (o+x) < 160){
					writePx(o+x, i+y, pixel, buffer);
				}
			}
		}
		
	}
	
}

void drawWireframeTriangle(unsigned short x1, unsigned short y1, unsigned short x2, unsigned short y2, unsigned short x3, unsigned short y3, unsigned short color, unsigned short* buffer){
	drawLine(x1, y1, x2, y2, color, buffer);
	drawLine(x1, y1, x3, y3, color, buffer);
	drawLine(x3, y3, x2, y2, color, buffer);
}

void drawFilledTriangle(unsigned short x1, unsigned short y1, unsigned short x2, unsigned short y2, unsigned short x3, unsigned short y3, unsigned short color){
	//get bounding box
	//iterate over all pixels in bounding box, check if within triangle; if so, draw
}

void clearScreen(unsigned short color, unsigned short* buffer){
	char x, y;
	for(x = 0; x<160;x++)   //loop through all x
	{
		for(y = 0; y<128; y++)  //loop through all y
		{
			//Screen[x+y*240] = RGB16(0,0,31);  
			writePx(x, y, color, buffer);
		}
	}
}

Triangle formTri(Vector vert1, Vector vert2, Vector vert3, unsigned short shade){
	Triangle temp;
	temp.vert1 = vert1;
	temp.vert2 = vert2;
	temp.vert3 = vert3;
	temp.shade = shade;
	return temp;
}

Entity formCube(unsigned short shade, float size){
	Entity temporary;
	Vector verts[8];
	temporary.numTris = 12;
	temporary.polygons = (Triangle*)malloc(temporary.numTris*sizeof(Triangle));
	int i;
	
	for(i=0;i<8;i++){
		if(((i%4)==0)||(i%4)==1){
			verts[i].x = size;
		}
		else{
			verts[i].x = -size;
		}
		if(i<4){
			verts[i].y = -size;
		}
		else{
			verts[i].y = size;
		}
		if( (i==7) || (i==4)  || (i==3) || (i==0) ){
			verts[i].z = -size;
		}
		else{
			verts[i].z = size;
		}
	}
	
	temporary.polygons[0] = formTri(verts[1], verts[2], verts[3], shade);
    temporary.polygons[1] = formTri(verts[7], verts[6], verts[5], shade);
    temporary.polygons[2] = formTri(verts[4], verts[5], verts[1], shade);
    temporary.polygons[3] = formTri(verts[5], verts[6], verts[2], shade);
    temporary.polygons[4] = formTri(verts[2], verts[6], verts[7], shade);
    temporary.polygons[5] = formTri(verts[0], verts[3], verts[7], shade);
    temporary.polygons[6] = formTri(verts[0], verts[1], verts[3], shade);
    temporary.polygons[7] = formTri(verts[4], verts[7], verts[5], shade);
    temporary.polygons[8] = formTri(verts[0], verts[4], verts[1], shade);
    temporary.polygons[9] = formTri(verts[1], verts[5], verts[2], shade);
    temporary.polygons[10] = formTri(verts[3], verts[2], verts[7], shade);
    temporary.polygons[11] = formTri(verts[4], verts[0], verts[7], shade);
	temporary.pos.x = 40;
	temporary.pos.y = 40;
	temporary.pos.z = 40;
	
	
	return temporary;
}

void display(Entity entity, unsigned short* surface) {
	float projection[3][3] = {
		{1, 0, 0},
		{0, 1, 0},
		{0, 0, 1},
	};


	float pitch = entity.rot.x;
	float yaw = entity.rot.y;
	float roll = entity.rot.z;

	float rotationX[3][3] = { {1, 0, 0},
		{0, cos(pitch * PI / 180), -sin(pitch * PI / 180)},
		{0, sin(pitch * PI / 180), cos(pitch * PI / 180)}
	};

	float rotationY[3][3] = { {cos(yaw * PI / 180), 0, -sin(yaw * PI / 180)},
		{0, 1, 0},
		{sin(yaw * PI / 180), 0, cos(yaw * PI / 180)}
	};

	float rotationZ[3][3] = { {cos(roll * PI / 180), -sin(roll * PI / 180), 0},
		{sin(roll * PI / 180), cos(roll * PI / 180), 0},
		{0, 0, 1}
	};

	int i = 0;
	int x1, x2, x3, y1, y2, y3;

	for (; i < entity.numTris; i++) {
		//display tri-by-tri
		float newFloats[3];
		float points[3] = { entity.polygons[i].vert1.x, entity.polygons[i].vert1.y, entity.polygons[i].vert1.z };
		float* temp = matmul(rotationX, points, newFloats);
		temp = matmul(rotationY, temp, newFloats);
		temp = matmul(rotationZ, temp, newFloats);
		temp = matmul(projection, temp, newFloats);

		x1 = temp[0] + entity.pos.x;
		y1 = temp[1] + entity.pos.y;
		//writePx(x1, y1, entity.polygons[i].shade, surface);

		float nextPoints[3] = { entity.polygons[i].vert2.x, entity.polygons[i].vert2.y, entity.polygons[i].vert2.z };
		temp = matmul(rotationX, nextPoints, newFloats);
		temp = matmul(rotationY, temp, newFloats);
		temp = matmul(rotationZ, temp, newFloats);
		temp = matmul(projection, temp, newFloats);
		x2 = temp[0] + entity.pos.x;
		y2 = temp[1] + entity.pos.y;
		//writePx(x2, y2, entity.polygons[i].shade, surface);

		float lastPoints[3] = { entity.polygons[i].vert3.x, entity.polygons[i].vert3.y, entity.polygons[i].vert3.z };
		temp = matmul(rotationX, lastPoints, newFloats);
		temp = matmul(rotationY, temp, newFloats);
		temp = matmul(rotationZ, temp, newFloats);
		temp = matmul(projection, temp, newFloats);
		x3 = temp[0] + entity.pos.x;
		y3 = temp[1] + entity.pos.y;
		//writePx(x3, y3, entity.polygons[i].shade, surface);

		if (state) { printf("%d %d %d %d %d %d\n", x1, y1, x2, y2, x3, y3); }
		drawWireframeTriangle(x1, y1, x2, y2, x3, y3, entity.polygons[i].shade, surface);
	}

	//state = 0;

}

int main()
{

	
	char x,y; 
	unsigned short* Screen = (unsigned short*)0x6000000; 
	*(unsigned long*)0x4000000 = 0x405; // mode3, bg2 on 

	unsigned short* buffer = pageFlip(backbuffer);
	
	Entity cube = formCube(RGB16(0, 31, 0), 20);
	int i = 10;
	
	while(1){
		display(cube, buffer);
		buffer = pageFlip(buffer);
		cube.rot.x++; cube.rot.y++; cube.rot.z++;
		memset(buffer, 0, 40960);	
	}
	
	/*float projection[3][3] = {
		{1, 0, 0},
		{0, 1, 0},
		{0, 0, 1},
	};
	
	Vector points[4];
	int o;
	
	points[0].x = -5;
	points[0].y = -5;
	
	points[1].x = 5;
	points[1].y = -5;
	
	points[2].x = -5;
	points[2].y = 5;
	
	points[3].x = 5;
	points[3].y = 5;
	
	
	int pitch = 0;
	int yaw = 0;
	int roll = 360;
	
	while(1){
	memset(buffer, 0, 40960);
	
	float rotationX[3][3] = {{1, 0, 0},
		{0, cos(pitch*PI/180), -sin(pitch*PI/180)},
		{0, sin(pitch*PI/180), cos(pitch*PI/180)}
	};
	
	float rotationY[3][3] = {{cos(yaw*PI/180), 0, -sin(yaw*PI/180)},
		{0, 1, 0},
		{sin(yaw*PI/180), 0, cos(yaw*PI/180)}
	};
	
	float rotationZ[3][3] = {{cos(roll*PI/180), -sin(roll*PI/180), 0},
		{sin(roll*PI/180), cos(roll*PI/180), 0},
		{0, 0, 1}
	};
	
	
	for(o = 0; o<4; o++){		
		//writePx(points[o].x, points[o].y, 65535, buffer);
		float tiff[3] = {points[o].x, points[o].y, points[o].z};
		float* jeff = matmul(rotationX, tiff);
		jeff = matmul(rotationY, jeff);
		jeff = matmul(rotationZ, jeff);
		float* biff = matmul(projection, jeff);
		writePx(biff[0]+30, biff[1]+30, 65535, buffer);
	}
	
	buffer = pageFlip(buffer);
	pitch++;
	//yaw++;
	//roll++;
	
	if(pitch>=360){pitch-=360;}
	if(yaw>=360){yaw-=360;}
	if(roll>=360){roll-=360;}
	
	}
	
	
	while(1){
	}
	
	// clear screen, and draw a blue back ground
	//clearScreen(RGB16(0, 0, 31), buffer);
	//memset(Screen, 35<<1, 40960);

	//asm("MOV r3, #0x6000000\nMOV r2, #0xFF000000\nSTR r2, [r3]");
	//drawWireframeTriangle(30, 30, 50, 30, 20, 50, 16383);
	//while(1){}	//loop forever*/
	
	//int posX = 30;
	
	/*while(1){
		memset(buffer, 35<<1, 40960);
		//clearScreen(RGB16(0, 0, 31), buffer);
		drawWireframeTriangle(posX, 30, 50, 30, 20, 50, 16383, buffer);
		posX++;
		if(posX>160){
			posX=0;
		}
		buffer = pageFlip(buffer);
	}*/
	
	/*short knockout[1024];
	memset(knockout, 255, 2048);
	int i;
	
	for(i = 0; i < 1024; i+=3){
		knockout[i] = 0;
	}*/
	
	/*while(1){
		memset(buffer, 35<<1, 40960);
		blitmap(posX, posX, 32, 32, knockout, buffer);
		buffer = pageFlip(buffer);
		posX++;
		if(posX>160){
			posX=0;
		}
	}*/
	
}
