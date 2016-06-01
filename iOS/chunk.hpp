//
//  chunk.hpp
//  templateApp
//
//  Created by Sangwook Nam on 6/28/14.
//  Copyright (c) 2014 ROm. All rights reserved.
//

#ifndef templateApp_chunk_hpp
#define templateApp_chunk_hpp

#include "glm.hpp"
#include "gfx.h"
#include "noise.hpp"
#include "matrix.hpp"
#include "matrix_transform.hpp"
#include "type_ptr.hpp"

//static GLint attribute_coord;
//static GLint uniform_mvp;
//
//static glm::vec3 position;
//static glm::vec3 forward;
//static glm::vec3 right;
//static glm::vec3 up;
//static glm::vec3 lookat;
//static glm::vec3 _angle;

static GLint attribute_coord;
static GLint uniform_mvp;
static GLuint texture;
static GLint uniform_texture;
static GLuint cursor_vbo;

static glm::vec3 position;
static glm::vec3 forward;
static glm::vec3 right;
static glm::vec3 up;
static glm::vec3 lookat;
static glm::vec3 _angle;
static time_t now;
static int ww, wh;

// Size of one chunk in blocks
#define CX 16
#define CY 32
#define CZ 16

// Number of chunks in the world
#define SCX 16
#define SCY 2
#define SCZ 16

// Sea level
#define SEALEVEL 4

// Number of VBO slots for chunks
#define CHUNKSLOTS (SCX * SCY * SCZ)

static const int transparent[16] = {2, 0, 0, 0, 1, 0, 0, 0, 3, 4, 0, 0, 0, 0, 0, 0};
static const char *blocknames[16] = {
	"air", "dirt", "topsoil", "grass", "leaves", "wood", "stone", "sand",
	"water", "glass", "brick", "ore", "woodrings", "white", "black", "x-y"
};



//struct byte4
//{
//    typedef signed char byte;
//	byte x;
//	byte y;
//	byte z;
//	byte t;
//
//    byte4() {};
//    byte4(byte x_, byte y_, byte z_, byte t_) : x(x_), y(y_), z(z_), t(t_) {}
//};


typedef glm::detail::tvec4<GLbyte, glm::defaultp> byte4;
static struct chunk *chunk_slot[CHUNKSLOTS] = {0};

struct chunk {
	uint8_t blk[CX][CY][CZ];
	struct chunk *left, *right, *below, *above, *front, *back;
	int slot;
	GLuint vbo;
	int elements;
	time_t lastused;
	bool changed;
	bool noised;
	bool initialized;
	int ax;
	int ay;
	int az;
    
	chunk(): ax(0), ay(0), az(0) {
		memset(blk, 0, sizeof blk);
		left = right = below = above = front = back = 0;
		lastused = now;
		slot = 0;
		changed = true;
		initialized = false;
		noised = false;
	}
    
	chunk(int x, int y, int z): ax(x), ay(y), az(z) {
		memset(blk, 0, sizeof blk);
		left = right = below = above = front = back = 0;
		lastused = now;
		slot = 0;
		changed = true;
		initialized = false;
		noised = false;
	}
    
	uint8_t get(int x, int y, int z) const {
		if(x < 0)
			return left ? left->blk[x + CX][y][z] : 0;
		if(x >= CX)
			return right ? right->blk[x - CX][y][z] : 0;
		if(y < 0)
			return below ? below->blk[x][y + CY][z] : 0;
		if(y >= CY)
			return above ? above->blk[x][y - CY][z] : 0;
		if(z < 0)
			return front ? front->blk[x][y][z + CZ] : 0;
		if(z >= CZ)
			return back ? back->blk[x][y][z - CZ] : 0;
		return blk[x][y][z];
	}
    
	bool isblocked(int x1, int y1, int z1, int x2, int y2, int z2) {
		// Invisible blocks are always "blocked"
		if(!blk[x1][y1][z1])
			return true;
        
		// Leaves do not block any other block, including themselves
		if(transparent[get(x2, y2, z2)] == 1)
			return false;
        
		// Non-transparent blocks always block line of sight
		if(!transparent[get(x2, y2, z2)])
			return true;
        
		// Otherwise, LOS is only blocked by blocks if the same transparency type
		return transparent[get(x2, y2, z2)] == transparent[blk[x1][y1][z1]];
	}
    
	void set(int x, int y, int z, uint8_t type) {
		// If coordinates are outside this chunk, find the right one.
		if(x < 0) {
			if(left)
				left->set(x + CX, y, z, type);
			return;
		}
		if(x >= CX) {
			if(right)
				right->set(x - CX, y, z, type);
			return;
		}
		if(y < 0) {
			if(below)
				below->set(x, y + CY, z, type);
			return;
		}
		if(y >= CY) {
			if(above)
				above->set(x, y - CY, z, type);
			return;
		}
		if(z < 0) {
			if(front)
				front->set(x, y, z + CZ, type);
			return;
		}
		if(z >= CZ) {
			if(back)
				back->set(x, y, z - CZ, type);
			return;
		}
        
		// Change the block
		blk[x][y][z] = type;
		changed = true;
        
		// When updating blocks at the edge of this chunk,
		// visibility of blocks in the neighbouring chunk might change.
		if(x == 0 && left)
			left->changed = true;
		if(x == CX - 1 && right)
			right->changed = true;
		if(y == 0 && below)
			below->changed = true;
		if(y == CY - 1 && above)
			above->changed = true;
		if(z == 0 && front)
			front->changed = true;
		if(z == CZ - 1 && back)
			back->changed = true;
	}
    
	static float noise2d(float x, float y, int seed, int octaves, float persistence) {
		float sum = 0;
		float strength = 1.0;
		float scale = 1.0;
        
		for(int i = 0; i < octaves; i++) {
			sum += strength * glm::simplex(glm::vec2(x, y) * scale);
			scale *= 2.0;
			strength *= persistence;
		}
        
		return sum;
	}
    
	static float noise3d_abs(float x, float y, float z, int seed, int octaves, float persistence) {
		float sum = 0;
		float strength = 1.0;
		float scale = 1.0;
        
		for(int i = 0; i < octaves; i++) {
			sum += strength * fabs(glm::simplex(glm::vec3(x, y, z) * scale));
			scale *= 2.0;
			strength *= persistence;
		}
        
		return sum;
	}
    
	void noise(int seed) {
		if(noised)
			return;
		else
			noised = true;
        
		for(int x = 0; x < CX; x++) {
			for(int z = 0; z < CZ; z++) {
				// Land height
				float n = noise2d((x + ax * CX) / 256.0, (z + az * CZ) / 256.0, seed, 5, 0.8) * 4;
				int h = n * 2;
				int y = 0;
                
				// Land blocks
				for(y = 0; y < CY; y++) {
					// Are we above "ground" level?
					if(y + ay * CY >= h) {
						// If we are not yet up to sea level, fill with water blocks
						if(y + ay * CY < SEALEVEL) {
							blk[x][y][z] = 8;
							continue;
                            // Otherwise, we are in the air
						} else {
							// A tree!
							if(get(x, y - 1, z) == 3 && (rand() & 0xff) == 0) {
								// Trunk
								h = (rand() & 0x3) + 3;
								for(int i = 0; i < h; i++)
									set(x, y + i, z, 5);
                                
								// Leaves
								for(int ix = -3; ix <= 3; ix++) {
									for(int iy = -3; iy <= 3; iy++) {
										for(int iz = -3; iz <= 3; iz++) {
											if(ix * ix + iy * iy + iz * iz < 8 + (rand() & 1) && !get(x + ix, y + h + iy, z + iz))
												set(x + ix, y + h + iy, z + iz, 4);
										}
									}
								}
							}
							break;
						}
					}
                    
					// Random value used to determine land type
					float r = noise3d_abs((x + ax * CX) / 16.0, (y + ay * CY) / 16.0, (z + az * CZ) / 16.0, -seed, 2, 1);
                    
					// Sand layer
					if(n + r * 5 < 4)
						blk[x][y][z] = 7;
					// Dirt layer, but use grass blocks for the top
					else if(n + r * 5 < 8)
						blk[x][y][z] = (h < SEALEVEL || y + ay * CY < h - 1) ? 1 : 3;
					// Rock layer
					else if(r < 1.25)
						blk[x][y][z] = 6;
					// Sometimes, ores!
					else
						blk[x][y][z] = 11;
				}
			}
		}
		changed = true;
	}
    
	void update() {
		byte4 vertex[CX * CY * CZ * 18];
		int i = 0;
		int merged = 0;
		bool vis = false;;
        
		// View from negative x
        
		for(int x = CX - 1; x >= 0; x--) {
			for(int y = 0; y < CY; y++) {
				for(int z = 0; z < CZ; z++) {
					// Line of sight blocked?
					if(isblocked(x, y, z, x - 1, y, z)) {
						vis = false;
						continue;
					}
                    
					uint8_t top = blk[x][y][z];
					uint8_t bottom = blk[x][y][z];
					uint8_t side = blk[x][y][z];
                    
					// Grass block has dirt sides and bottom
					if(top == 3) {
						bottom = 1;
						side = 2;
                        // Wood blocks have rings on top and bottom
					} else if(top == 5) {
						top = bottom = 12;
					}
                    
					// Same block as previous one? Extend it.
					if(vis && z != 0 && blk[x][y][z] == blk[x][y][z - 1]) {
						vertex[i - 5] = byte4(x, y, z + 1, side);
						vertex[i - 2] = byte4(x, y, z + 1, side);
						vertex[i - 1] = byte4(x, y + 1, z + 1, side);
						merged++;
                        // Otherwise, add a new quad.
					} else {
						vertex[i++] = byte4(x, y, z, side);
						vertex[i++] = byte4(x, y, z + 1, side);
						vertex[i++] = byte4(x, y + 1, z, side);
						vertex[i++] = byte4(x, y + 1, z, side);
						vertex[i++] = byte4(x, y, z + 1, side);
						vertex[i++] = byte4(x, y + 1, z + 1, side);
					}
					
					vis = true;
				}
			}
		}
        
		// View from positive x
        
		for(int x = 0; x < CX; x++) {
			for(int y = 0; y < CY; y++) {
				for(int z = 0; z < CZ; z++) {
					if(isblocked(x, y, z, x + 1, y, z)) {
						vis = false;
						continue;
					}
                    
					uint8_t top = blk[x][y][z];
					uint8_t bottom = blk[x][y][z];
					uint8_t side = blk[x][y][z];
                    
					if(top == 3) {
						bottom = 1;
						side = 2;
					} else if(top == 5) {
						top = bottom = 12;
					}
                    
					if(vis && z != 0 && blk[x][y][z] == blk[x][y][z - 1]) {
						vertex[i - 4] = byte4(x + 1, y, z + 1, side);
						vertex[i - 2] = byte4(x + 1, y + 1, z + 1, side);
						vertex[i - 1] = byte4(x + 1, y, z + 1, side);
						merged++;
					} else {
						vertex[i++] = byte4(x + 1, y, z, side);
						vertex[i++] = byte4(x + 1, y + 1, z, side);
						vertex[i++] = byte4(x + 1, y, z + 1, side);
						vertex[i++] = byte4(x + 1, y + 1, z, side);
						vertex[i++] = byte4(x + 1, y + 1, z + 1, side);
						vertex[i++] = byte4(x + 1, y, z + 1, side);
					}
					vis = true;
				}
			}
		}
        
		// View from negative y
        
		for(int x = 0; x < CX; x++) {
			for(int y = CY - 1; y >= 0; y--) {
				for(int z = 0; z < CZ; z++) {
					if(isblocked(x, y, z, x, y - 1, z)) {
						vis = false;
						continue;
					}
                    
					uint8_t top = blk[x][y][z];
					uint8_t bottom = blk[x][y][z];
                    
					if(top == 3) {
						bottom = 1;
					} else if(top == 5) {
						top = bottom = 12;
					}
                    
					if(vis && z != 0 && blk[x][y][z] == blk[x][y][z - 1]) {
						vertex[i - 4] = byte4(x, y, z + 1, bottom + 128);
						vertex[i - 2] = byte4(x + 1, y, z + 1, bottom + 128);
						vertex[i - 1] = byte4(x, y, z + 1, bottom + 128);
						merged++;
					} else {
						vertex[i++] = byte4(x, y, z, bottom + 128);
						vertex[i++] = byte4(x + 1, y, z, bottom + 128);
						vertex[i++] = byte4(x, y, z + 1, bottom + 128);
						vertex[i++] = byte4(x + 1, y, z, bottom + 128);
						vertex[i++] = byte4(x + 1, y, z + 1, bottom + 128);
						vertex[i++] = byte4(x, y, z + 1, bottom + 128);
					}
					vis = true;
				}
			}
		}
        
		// View from positive y
        
		for(int x = 0; x < CX; x++) {
			for(int y = 0; y < CY; y++) {
				for(int z = 0; z < CZ; z++) {
					if(isblocked(x, y, z, x, y + 1, z)) {
						vis = false;
						continue;
					}
                    
					uint8_t top = blk[x][y][z];
					uint8_t bottom = blk[x][y][z];
                    
					if(top == 3) {
						bottom = 1;
					} else if(top == 5) {
						top = bottom = 12;
					}
                    
					if(vis && z != 0 && blk[x][y][z] == blk[x][y][z - 1]) {
						vertex[i - 5] = byte4(x, y + 1, z + 1, top + 128);
						vertex[i - 2] = byte4(x, y + 1, z + 1, top + 128);
						vertex[i - 1] = byte4(x + 1, y + 1, z + 1, top + 128);
						merged++;
					} else {
						vertex[i++] = byte4(x, y + 1, z, top + 128);
						vertex[i++] = byte4(x, y + 1, z + 1, top + 128);
						vertex[i++] = byte4(x + 1, y + 1, z, top + 128);
						vertex[i++] = byte4(x + 1, y + 1, z, top + 128);
						vertex[i++] = byte4(x, y + 1, z + 1, top + 128);
						vertex[i++] = byte4(x + 1, y + 1, z + 1, top + 128);
					}
					vis = true;
				}
			}
		}
        
		// View from negative z
        
		for(int x = 0; x < CX; x++) {
			for(int z = CZ - 1; z >= 0; z--) {
				for(int y = 0; y < CY; y++) {
					if(isblocked(x, y, z, x, y, z - 1)) {
						vis = false;
						continue;
					}
                    
					uint8_t top = blk[x][y][z];
					uint8_t bottom = blk[x][y][z];
					uint8_t side = blk[x][y][z];
                    
					if(top == 3) {
						bottom = 1;
						side = 2;
					} else if(top == 5) {
						top = bottom = 12;
					}
                    
					if(vis && y != 0 && blk[x][y][z] == blk[x][y - 1][z]) {
						vertex[i - 5] = byte4(x, y + 1, z, side);
						vertex[i - 3] = byte4(x, y + 1, z, side);
						vertex[i - 2] = byte4(x + 1, y + 1, z, side);
						merged++;
					} else {
						vertex[i++] = byte4(x, y, z, side);
						vertex[i++] = byte4(x, y + 1, z, side);
						vertex[i++] = byte4(x + 1, y, z, side);
						vertex[i++] = byte4(x, y + 1, z, side);
						vertex[i++] = byte4(x + 1, y + 1, z, side);
						vertex[i++] = byte4(x + 1, y, z, side);
					}
					vis = true;
				}
			}
		}
        
		// View from positive z
        
		for(int x = 0; x < CX; x++) {
			for(int z = 0; z < CZ; z++) {
				for(int y = 0; y < CY; y++) {
					if(isblocked(x, y, z, x, y, z + 1)) {
						vis = false;
						continue;
					}
                    
					uint8_t top = blk[x][y][z];
					uint8_t bottom = blk[x][y][z];
					uint8_t side = blk[x][y][z];
                    
					if(top == 3) {
						bottom = 1;
						side = 2;
					} else if(top == 5) {
						top = bottom = 12;
					}
                    
					if(vis && y != 0 && blk[x][y][z] == blk[x][y - 1][z]) {
						vertex[i - 4] = byte4(x, y + 1, z + 1, side);
						vertex[i - 3] = byte4(x, y + 1, z + 1, side);
						vertex[i - 1] = byte4(x + 1, y + 1, z + 1, side);
						merged++;
					} else {
						vertex[i++] = byte4(x, y, z + 1, side);
						vertex[i++] = byte4(x + 1, y, z + 1, side);
						vertex[i++] = byte4(x, y + 1, z + 1, side);
						vertex[i++] = byte4(x, y + 1, z + 1, side);
						vertex[i++] = byte4(x + 1, y, z + 1, side);
						vertex[i++] = byte4(x + 1, y + 1, z + 1, side);
					}
					vis = true;
				}
			}
		}
        
		changed = false;
		elements = i;
        
		// If this chunk is empty, no need to allocate a chunk slot.
		if(!elements)
			return;
        
		// If we don't have an active slot, find one
		if(chunk_slot[slot] != this) {
			int lru = 0;
			for(int i = 0; i < CHUNKSLOTS; i++) {
				// If there is an empty slot, use it
				if(!chunk_slot[i]) {
					lru = i;
					break;
				}
				// Otherwise try to find the least recently used slot
				if(chunk_slot[i]->lastused < chunk_slot[lru]->lastused)
					lru = i;
			}
            
			// If the slot is empty, create a new VBO
			if(!chunk_slot[lru]) {
				glGenBuffers(1, &vbo);
                // Otherwise, steal it from the previous slot owner
			} else {
				vbo = chunk_slot[lru]->vbo;
				chunk_slot[lru]->changed = true;
			}
            
			slot = lru;
			chunk_slot[slot] = this;
		}
        
		// Upload vertices
        
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, i * sizeof *vertex, vertex, GL_STATIC_DRAW);
	}
    
	void render() {
		if(changed)
			update();
        
		lastused = now;
        
		if(!elements)
			return;
        
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glVertexAttribPointer(attribute_coord, 4, GL_BYTE, GL_FALSE, 0, 0);
		glDrawArrays(GL_TRIANGLES, 0, elements);
	}
};

struct superchunk {
	chunk *c[SCX][SCY][SCZ];
	time_t seed;
    
	superchunk() {
		seed = time(NULL);
		for(int x = 0; x < SCX; x++)
			for(int y = 0; y < SCY; y++)
				for(int z = 0; z < SCZ; z++)
					c[x][y][z] = new chunk(x - SCX / 2, y - SCY / 2, z - SCZ / 2);
        
		for(int x = 0; x < SCX; x++)
			for(int y = 0; y < SCY; y++)
				for(int z = 0; z < SCZ; z++) {
					if(x > 0)
						c[x][y][z]->left = c[x - 1][y][z];
					if(x < SCX - 1)
						c[x][y][z]->right = c[x + 1][y][z];
					if(y > 0)
						c[x][y][z]->below = c[x][y - 1][z];
					if(y < SCY - 1)
						c[x][y][z]->above = c[x][y + 1][z];
					if(z > 0)
						c[x][y][z]->front = c[x][y][z - 1];
					if(z < SCZ - 1)
						c[x][y][z]->back = c[x][y][z + 1];
				}
	}
    
	uint8_t get(int x, int y, int z) const {
		int cx = (x + CX * (SCX / 2)) / CX;
		int cy = (y + CY * (SCY / 2)) / CY;
		int cz = (z + CZ * (SCZ / 2)) / CZ;
        
		if(cx < 0 || cx >= SCX || cy < 0 || cy >= SCY || cz <= 0 || cz >= SCZ)
			return 0;
        
		return c[cx][cy][cz]->get(x & (CX - 1), y & (CY - 1), z & (CZ - 1));
	}
    
	void set(int x, int y, int z, uint8_t type) {
		int cx = (x + CX * (SCX / 2)) / CX;
		int cy = (y + CY * (SCY / 2)) / CY;
		int cz = (z + CZ * (SCZ / 2)) / CZ;
        
		if(cx < 0 || cx >= SCX || cy < 0 || cy >= SCY || cz <= 0 || cz >= SCZ)
			return;
        
		c[cx][cy][cz]->set(x & (CX - 1), y & (CY - 1), z & (CZ - 1), type);
	}
    
	void render(const glm::mat4 &pv) {
		float ud = 1.0 / 0.0;
		int ux = -1;
		int uy = -1;
		int uz = -1;
        
		for(int x = 0; x < SCX; x++) {
			for(int y = 0; y < SCY; y++) {
				for(int z = 0; z < SCZ; z++) {
					glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(c[x][y][z]->ax * CX, c[x][y][z]->ay * CY, c[x][y][z]->az * CZ));
					glm::mat4 mvp = pv * model;
                    
					// Is this chunk on the screen?
					glm::vec4 center = mvp * glm::vec4(CX / 2, CY / 2, CZ / 2, 1);
                    
					float d = glm::length(center);
					center.x /= center.w;
					center.y /= center.w;
                    
					// If it is behind the camera, don't bother drawing it
					if(center.z < -CY / 2)
						continue;
                    
					// If it is outside the screen, don't bother drawing it
					if(fabsf(center.x) > 1 + fabsf(CY * 2 / center.w) || fabsf(center.y) > 1 + fabsf(CY * 2 / center.w))
						continue;
                    
					// If this chunk is not initialized, skip it
					if(!c[x][y][z]->initialized) {
						// But if it is the closest to the camera, mark it for initialization
						if(ux < 0 || d < ud) {
							ud = d;
							ux = x;
							uy = y;
							uz = z;
						}
						continue;
					}
                    
					glUniformMatrix4fv(uniform_mvp, 1, GL_FALSE, glm::value_ptr(mvp));
                    
					c[x][y][z]->render();
				}
			}
		}
        
		if(ux >= 0) {
			c[ux][uy][uz]->noise(seed);
			if(c[ux][uy][uz]->left)
				c[ux][uy][uz]->left->noise(seed);
			if(c[ux][uy][uz]->right)
				c[ux][uy][uz]->right->noise(seed);
			if(c[ux][uy][uz]->below)
				c[ux][uy][uz]->below->noise(seed);
			if(c[ux][uy][uz]->above)
				c[ux][uy][uz]->above->noise(seed);
			if(c[ux][uy][uz]->front)
				c[ux][uy][uz]->front->noise(seed);
			if(c[ux][uy][uz]->back)
				c[ux][uy][uz]->back->noise(seed);
			c[ux][uy][uz]->initialized = true;
		}
	}
};


static superchunk *world;

// Calculate the forward, right and lookat vectors from the angle vector
static void update_vectors() {
	forward.x = sinf(_angle.x);
	forward.y = 0;
	forward.z = cosf(_angle.x);
    
	right.x = -cosf(_angle.x);
	right.y = 0;
	right.z = sinf(_angle.x);
    
	lookat.x = sinf(_angle.x) * cosf(_angle.y);
	lookat.y = sinf(_angle.y);
	lookat.z = cosf(_angle.x) * cosf(_angle.y);
    
	up = glm::cross(right, lookat);
}


#endif
