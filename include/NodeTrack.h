/*
 *  NodeTrack.h
 *  Bloom
 *
 *  Created by Robert Hodgin on 1/21/11.
 *  Copyright 2011 __MyCompanyName__. All rights reserved.
 *
 */

#pragma once

#include "Node.h"
#include "cinder/Vector.h"

class NodeTrack : public Node
{
  public:
	NodeTrack( ci::ipod::Player *player, Node *parent, int index, vector<ci::Font*> fonts, std::string name );

	void update( const ci::Matrix44f &mat, const ci::Vec3f &bbRight, const ci::Vec3f &bbUp );
	void drawSphere( std::vector<ci::gl::Texture*> texs );
	void drawRings( std::vector<ci::gl::Texture*> texs );
	void select();
	void setData( ci::ipod::TrackRef track, ci::ipod::PlaylistRef album );
	
	ci::ipod::PlaylistRef mAlbum;
	ci::ipod::TrackRef mTrack;
	float mTrackLength;
	float mPlayCount;
	double lastTime;
	
	int mSphereRes;
	
	int mTotalVerts;
	GLfloat *mVerts;
	GLfloat *mTexCoords;
	
	ci::Color mAtmosphereColor;
	
	float mAxisAngle;
	
	bool mIsPlaying;
};