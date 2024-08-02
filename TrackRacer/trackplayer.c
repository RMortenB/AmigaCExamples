#include <stdlib.h>
#include <string.h>
#include <proto/lowlevel.h>
#include "vec3.h"
#include "trackracer.h"

/*****************************************************************************/

#define MAX_TRIANGLES 32

/*****************************************************************************/

typedef struct
{
	vec3	pos;
	vec3	vel;
	mat4	orientation;
	mat4	momentum;
} tr_rigid;

/*****************************************************************************/

struct tr_player
{
	tr_rigid rigid;
	mat4 trackAlign;
	float trackPos;
	float angle;
};

/*****************************************************************************/

void TR_GetPlayerPos(tr_player *player, vec3 *pos)
{
	*pos = player->rigid.pos;
}

/*****************************************************************************/

void TR_GetPlayerOrientation(const tr_player *player, vec3 *dir)
{
	vec3 fwd = {0.0f, 0.0f, 1.0f};
	mat4 align;
	mat4_mul(&align, &player->trackAlign, &player->rigid.orientation);
	vec3_tform(dir, &align, &fwd, 0.0f);
}

/*****************************************************************************/

void DampenPlayer(tr_player *player)
{
	mat4 ident;
	mat4_identity(&ident);
	for(int i = 0; i < 4; ++i)
	{
		for(int j = 0; j < 4; ++j)
		{
			player->rigid.momentum.m[i][j] = player->rigid.momentum.m[i][j] * 0.95f + ident.m[i][j] * 0.05f;
		}
	}
	mat4_sync(&player->rigid.orientation);
	mat4_sync(&player->rigid.momentum);

	vec3_scale(&player->rigid.vel, &player->rigid.vel, 0.97f);
}

/*****************************************************************************/

float vec3_tripple(vec3 *a, vec3 *b, vec3 *c)
{
	vec3 axb;
	vec3_cross(&axb, a, b);
	return vec3_dot(&axb, c);
}

/*****************************************************************************/

static void tr_ClosestPointTriangle2(vec3 *barycentric, vec3 *pos, tr_collvertex *a, tr_collvertex *b, tr_collvertex *c)
{
	vec3 ab, ac, ap;
	vec3_sub(&ab, &b->pos, &a->pos);
	vec3_sub(&ac, &c->pos, &a->pos);
	vec3_sub(&ap, pos, &a->pos);

	float d1 = vec3_dot(&ab, &ap);
	float d2 = vec3_dot(&ac, &ap);

	if(d1 <= 0.0f && d2 <= 0.0f)
	{
		vec3_set(barycentric, 1.0f, 0.0f, 0.0f);
		return;
	}
	vec3 bp;
	vec3_sub(&bp, pos, &b->pos);
	float d3 = vec3_dot(&ab, &bp);
	float d4 = vec3_dot(&ac, &bp);
	if(d3 >= 0.0f && d4 <= d3)
	{
		vec3_set(barycentric, 0.0f, 1.0f, 0.0f);
		return;
	}
	float vc = d1 * d4 - d3 * d2;
	if(vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f)
	{
		float v = d1 / (d1 - d3);
		vec3_set(barycentric, 1.0f - v, v, 0.0f);
		return;
	}
	vec3 cp;
	vec3_sub(&cp, pos, &c->pos);
	float d5 = vec3_dot(&ab, &cp);
	float d6 = vec3_dot(&ac, &cp);
	if(d6 >= 0.0f && d5 <= d6)
	{
		vec3_set(barycentric, 0.0f, 0.0f, 1.0f);
		return;
	}
	float vb = d5 * d2 - d1 * d6;
	if(vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f)
	{
		float w = d2 / (d2 - d6);
		vec3_set(barycentric, 1.0f - w, 0.0f, w);
		return;
	}
	float va = d3 * d6 - d5 * d4;
	if(va <= 0.0f && (d4 - d3) >= 0.0f && (d5 - d6) >= 0.0f)
	{
		float w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
		vec3_set(barycentric, 0.0f, 1.0f - w, w);
		return;
	}
	float denom = 1.0f / (va + vb + vc);
	float v = vb * denom;
	float w = vc * denom;

	vec3_set(barycentric, 1.0f - v - w, v, w);
	return;
}

/*****************************************************************************/

float TR_ClosestPointTriangle(tr_collvertex *res, vec3 *pos, tr_collvertex *a, tr_collvertex *b, tr_collvertex *c)
{
	vec3 barycentric;
	tr_ClosestPointTriangle2(&barycentric, pos, a, b, c);
	vec3 p0, p1, p2;
	vec3_scale(&p0, &a->pos, barycentric.x);
	vec3_scale(&p1, &b->pos, barycentric.y);
	vec3_scale(&p2, &c->pos, barycentric.z);
	vec3_add(&res->pos, &p0, &p1);
	vec3_add(&res->pos, &res->pos, &p2);
	res->spos = a->spos * barycentric.x + b->spos * barycentric.y + c->spos * barycentric.z;
	vec3 lvec;

	vec3_sub(&lvec, &res->pos, pos);
	return vec3_len(&lvec);
}

/*****************************************************************************/

int TR_GetClosestPoint(tr_collvertex *res, tr_track *track, vec3 *pos, float radius)
{
	tr_collvertex trackTris[3 * MAX_TRIANGLES];
	tr_aabb aabb;
	aabb.bbmin = *pos;
	aabb.bbmax = *pos;

	vec3 size = {radius, radius, radius};
	vec3_sub(&aabb.bbmin, &aabb.bbmin, &size);
	vec3_add(&aabb.bbmax, &aabb.bbmax, &size);

	int nTris = TR_GetTrianglesInAABB(track, trackTris, MAX_TRIANGLES, &aabb);

	if(!nTris)
	{
		return 0;
	}
	int rVal = 0;
	float minDist = radius;

	for(int i = 0; i < nTris; ++i)
	{
		tr_collvertex closest;
		float distance = TR_ClosestPointTriangle(&closest, pos, &trackTris[i * 3 + 0], &trackTris[i * 3 + 1], &trackTris[i * 3 + 2]);
		if(minDist > distance)
		{
			minDist = distance;
			*res = closest;
			rVal = 1;
		}
	}
	return rVal;
}

/*****************************************************************************/

void TR_CollidePlayerTrack(tr_track *track, tr_player *player)
{
	vec3 front = {0.0f, 0.0f, 0.1f};
	vec3 back = {0.0f, 0.0f, -0.1f};
	vec3_tform(&front, &player->rigid.orientation, &front, 0.0f);
	vec3_tform(&back, &player->rigid.orientation, &back, 0.0f);
	vec3_add(&front, &front, &player->rigid.pos);
	vec3_add(&back, &back, &player->rigid.pos);

	int applyGravity = 1;

	tr_collvertex closest;
	if(TR_GetClosestPoint(&closest, track, &player->rigid.pos, 0.2f))
	{
		vec3 rel;
		player->trackPos = closest.spos;
		vec3_sub(&rel, &closest.pos, &player->rigid.pos);
		float dist = vec3_len(&rel);
		if(dist < 0.2f)
		{
			vec3 deflect;
			vec3_normalise(&deflect, &rel);
			vec3_scale(&deflect, &deflect, 0.2f - dist);
			vec3_sub(&player->rigid.pos, &player->rigid.pos, &deflect);
			applyGravity = 0;
		}
	}
	if(applyGravity)
	{
		vec3 gravity = {0.0f, -0.0025f, 0.0f};
		vec3_add(&player->rigid.vel, &player->rigid.vel, &gravity);
	}
}

/*****************************************************************************/
static vec3 currentAlign;
static vec3 currentDir;

void TR_UpdatePlayer(tr_track *track, tr_player *player)
{
	uint32_t joyPort1 = ReadJoyPort(1);

	vec3 trackPos, trackDir;

	TR_ResolvePoint(&trackPos, &trackDir, track, player->trackPos);

	vec3_normalise(&currentDir, &trackDir);

	vec3 xDir, yDir, zDir;
	vec3 up = {0.0f, 1.0f, 0.0f};

	vec3_normalise(&zDir, &trackDir);
	vec3_cross(&xDir, &up, &zDir);
	vec3_normalise(&xDir, &xDir);
	vec3_cross(&yDir, &zDir, &xDir);
	vec3_set(&zDir, 0.0f, 0.0f, 1.0f);
	vec3_cross(&xDir, &yDir, &zDir);
	vec3_normalise(&xDir, &xDir);
	vec3_cross(&zDir, &xDir, &yDir);
	vec3_normalise(&zDir, &zDir);

	mat4_identity(&player->trackAlign);

	player->trackAlign.m[0][0] = xDir.x;
	player->trackAlign.m[0][1] = xDir.y;
	player->trackAlign.m[0][2] = xDir.z;
	player->trackAlign.m[1][0] = yDir.x;
	player->trackAlign.m[1][1] = yDir.y;
	player->trackAlign.m[1][2] = yDir.z;
	player->trackAlign.m[2][0] = zDir.x;
	player->trackAlign.m[2][1] = zDir.y;
	player->trackAlign.m[2][2] = zDir.z;

	TR_GetPlayerOrientation(player, &currentAlign);

	if(joyPort1 & JPF_BUTTON_RED)
	{
		vec3 impulse;
		TR_GetPlayerOrientation(player, &impulse);
		vec3_scale(&impulse, &impulse, 0.005f);
		vec3_add(&player->rigid.vel, &player->rigid.vel, &impulse);
	}
	if(joyPort1 & JPF_JOY_DOWN)
	{
		vec3 impulse;
		TR_GetPlayerOrientation(player, &impulse);
		vec3_scale(&impulse, &impulse, -0.002f);
		vec3_add(&player->rigid.vel, &player->rigid.vel, &impulse);
	}
	if(joyPort1 & JPF_JOY_LEFT)
	{
		mat4 impulse;
		mat4_rotateY(&impulse, -0.0025f);
		mat4_mul(&player->rigid.momentum, &player->rigid.momentum, &impulse);
	}
	if(joyPort1 & JPF_JOY_RIGHT)
	{
		mat4 impulse;
		mat4_rotateY(&impulse, 0.0025f);
		mat4_mul(&player->rigid.momentum, &player->rigid.momentum, &impulse);
	}

	mat4_mul(&player->rigid.orientation, &player->rigid.orientation, &player->rigid.momentum);
	vec3_add(&player->rigid.pos, &player->rigid.pos, &player->rigid.vel);

	TR_CollidePlayerTrack(track, player);

	DampenPlayer(player);
}

/*****************************************************************************/

void DrawCross(MaggieFormat *pixels, int xPos, int yPos)
{
	if(xPos < -5) return;
	if(yPos < -5) return;
	if(xPos >= (MAGGIE_XRES + 5)) return;
	if(yPos >= (MAGGIE_YRES + 5)) return;

	for(int i = 0; i < 10; ++i)
	{
		int x, y;
		x = xPos + i - 5;
		y = yPos;
		if(x < 0) continue;
		if(y < 0) continue;
		if(x >= MAGGIE_XRES) continue;
		if(y >= MAGGIE_YRES) continue;
		pixels[y * MAGGIE_XRES + x] = ~0;
		x = xPos;
		y = yPos + i - 5;
		if(x < 0) continue;
		if(y < 0) continue;
		if(x >= MAGGIE_XRES) continue;
		if(y >= MAGGIE_YRES) continue;
		pixels[y * MAGGIE_XRES + x] = ~0;
	}
}

/*****************************************************************************/

void TR_DrawPlayer(MaggieFormat *pixels, tr_player *player, mat4 *modelView, mat4 *projection)
{
	mat4 viewProj;

	mat4_mul(&viewProj, projection, modelView);

	mat4 playerTrans = player->trackAlign;//player->rigid.orientation;
	mat4 iAlign;
	mat4_inverseLight(&iAlign, &player->trackAlign);
	mat4_mul(&playerTrans, &player->trackAlign, &player->rigid.orientation);

//	mat4 playerTrans = player->trackAlign;//player->rigid.orientation;
	playerTrans.m[3][0] = player->rigid.pos.x;
	playerTrans.m[3][1] = player->rigid.pos.y;
	playerTrans.m[3][2] = player->rigid.pos.z;

	mat4_mul(&playerTrans, &viewProj, &playerTrans);
	vec4 playerPos[3];
	vec3 center = {0.0f, 0.0f, 0.0f};
	vec3 fwd = { 0.0f, 0.0f, 0.2f };
	vec3 bck = { 0.0f, 0.0f, -0.2f };
	
	vec3_tformh(&playerPos[0], &playerTrans, &fwd, 1.0f);
	vec3_tformh(&playerPos[1], &playerTrans, &bck, 1.0f);
	vec3_tformh(&playerPos[2], &playerTrans, &center, 1.0f);
	for(int i = 0; i < 3; ++i)
	{
		if(-playerPos[i].w > playerPos[i].x) return;
		if( playerPos[i].x > playerPos[i].w) return;
		if(-playerPos[i].w > playerPos[i].y) return;
		if( playerPos[i].y > playerPos[i].w) return;
		if(           0.0f > playerPos[i].z) return;
		if( playerPos[i].x > playerPos[i].w) return;

		int xPos = (playerPos[i].x / playerPos[i].w + 1.0f) * 0.5f * MAGGIE_XRES;
		int yPos = (playerPos[i].y / playerPos[i].w + 1.0f) * 0.5f * MAGGIE_YRES;

		DrawCross(pixels, xPos, yPos);
	}
	TextOut(pixels, 0, 0, "trackPos %f", player->trackPos);

	TextOut(pixels, 0,  8, "[ %.4f %.4f %.4f ]", player->trackAlign.m[0][0], player->trackAlign.m[0][1], player->trackAlign.m[0][2]);
	TextOut(pixels, 0, 16, "[ %.4f %.4f %.4f ]", player->trackAlign.m[1][0], player->trackAlign.m[1][1], player->trackAlign.m[1][2]);
	TextOut(pixels, 0, 24, "[ %.4f %.4f %.4f ]", player->trackAlign.m[2][0], player->trackAlign.m[2][1], player->trackAlign.m[2][2]);

	TextOut(pixels, 0, 32, "Angle : %.4f", vec3_dot(&currentAlign, &currentDir));
}

/*****************************************************************************/
/*****************************************************************************/

tr_player *TR_CreatePlayer(tr_track *track)
{
	tr_player *player = malloc(sizeof(tr_player));
	memset(player, 0, sizeof(tr_player));

	mat4_identity(&player->rigid.orientation);
	mat4_identity(&player->rigid.momentum);
	vec3 up = { 0.0f, 1.0f, 0.0f };
	vec3 xVec, yVec, zVec;

	TR_ResolvePoint(&player->rigid.pos, &zVec, track, 0.0f);
	player->rigid.pos.y += 0.01f;
	vec3_normalise(&zVec, &zVec);
	vec3_cross(&xVec, &up, &zVec);
	vec3_normalise(&xVec, &xVec);
	vec3_cross(&yVec, &zVec, &xVec);

	player->rigid.orientation.m[0][0] = xVec.x;
	player->rigid.orientation.m[0][1] = xVec.y;
	player->rigid.orientation.m[0][2] = xVec.z;
	player->rigid.orientation.m[0][3] = 0.0f;
	player->rigid.orientation.m[1][0] = yVec.x;
	player->rigid.orientation.m[1][1] = yVec.y;
	player->rigid.orientation.m[1][2] = yVec.z;
	player->rigid.orientation.m[1][3] = 0.0f;
	player->rigid.orientation.m[2][0] = zVec.x;
	player->rigid.orientation.m[2][1] = zVec.y;
	player->rigid.orientation.m[2][2] = zVec.z;
	player->rigid.orientation.m[2][3] = 0.0f;
	player->rigid.orientation.m[3][0] = 0.0f;
	player->rigid.orientation.m[3][1] = 0.0f;
	player->rigid.orientation.m[3][2] = 0.0f;
	player->rigid.orientation.m[3][3] = 1.0f;

	return player;
}

/*****************************************************************************/
