#include "cinder/app/AppCocoaTouch.h"
#include "cinder/app/Renderer.h"
#include "cinder/Surface.h"
#include "cinder/gl/Texture.h"
#include "cinder/Camera.h"
#include "cinder/Font.h"
#include "cinder/Arcball.h"
#include "cinder/ImageIo.h"
#include "cinder/Rand.h"
#include "Globals.h"
#include "Easing.h"
#include "World.h"
#include "UiLayer.h"
#include "State.h"
#include "Data.h"
#include "PlayControls.h"
#include "Breadcrumbs.h"
#include "BreadcrumbEvent.h"
#include "CinderIPod.h"
#include "CinderIPodPlayer.h"
#include "PinchRecognizer.h"
#include <vector>
#include <sstream>

using std::vector;
using namespace ci;
using namespace ci::app;
using namespace std;
using std::stringstream;

//int G_CURRENT_LEVEL		= 0;
float G_ZOOM			= 0;
bool G_DEBUG			= false;
GLfloat mat_ambient[]	= { 0.02, 0.02, 0.05, 1.0 };
GLfloat mat_diffuse[]	= { 1.0, 1.0, 1.0, 1.0 };
GLfloat mat_specular[]	= { 1.0, 1.0, 1.0, 1.0 };
GLfloat mat_shininess[]	= { 100.0 };

float easeInOutQuad( double t, float b, float c, double d );
Vec3f easeInOutQuad( double t, Vec3f b, Vec3f c, double d );

class KeplerApp : public AppCocoaTouch {
  public:
	void			init();
	virtual void	setup();
	virtual void	touchesBegan( TouchEvent event );
	virtual void	touchesMoved( TouchEvent event );
	virtual void	touchesEnded( TouchEvent event );
	virtual void	accelerated( AccelEvent event );
	void			initTextures();
	virtual void	update();
	void			updateArcball();
	void			updateCamera();
	void			updatePlayhead();
	virtual void	draw();
	void			drawInfoPanel();
	void			setParamsTex();
	bool			onAlphaCharStateChanged( State *state );
	bool			onAlphaCharSelected( UiLayer *uiLayer );
	bool			onWheelClosed( UiLayer *uiLayer );
	bool			onBreadcrumbSelected ( BreadcrumbEvent event );
	bool			onPlayControlsButtonPressed ( PlayControls::PlayButton button );
	bool			onNodeSelected( Node *node );
	void			checkForNodeTouch( const Ray &ray, Matrix44f &mat );
	bool			onPlayerStateChanged( ipod::Player *player );
    bool			onPlayerTrackChanged( ipod::Player *player );
	
	World			mWorld;
	State			mState;
	UiLayer			mUiLayer;
	Data			mData;
	
	// ACCELEROMETER
	Matrix44f		mAccelMatrix;
	Matrix44f		mNewAccelMatrix;
	
	// AUDIO
	ipod::Player		mIpodPlayer;
	ipod::PlaylistRef	mCurrentAlbum;
	double				mCurrentTrackPlayheadTime;	
//	int					mCurrentTrackId;

	
	// BREADCRUMBS
	Breadcrumbs		mBreadcrumbs;	
	
	// PLAY CONTROLS
	PlayControls	mPlayControls;
	
	// CAMERA PERSP
	CameraPersp		mCam;
	float			mFov, mFovDest;
	Vec3f			mEye, mCenter, mUp;
	Vec3f			mCamVel;
	Vec3f			mCenterDest, mCenterFrom;
	float			mCamDist, mCamDistDest, mCamDistFrom;
	float			mZoomFrom, mZoomDest;
	Arcball			mArcball;
	Matrix44f		mMatrix;
	Vec3f			mBbRight, mBbUp;
	
	
	// FONTS
	Font			mFont;
	Font			mFontBig;
	
	
	// MULTITOUCH
	Vec2f			mTouchPos;
	Vec2f			mTouchThrowVel;
	Vec2f			mTouchVel;
	bool			mIsDragging;
	bool			mIsPinching;
	bool			onPinchBegan( PinchEvent event );
    bool			onPinchMoved( PinchEvent event );
    bool			onPinchEnded( PinchEvent event );
    vector<Ray>		mPinchRays;
    PinchRecognizer mPinchRecognizer;

	
	
	// TEXTURES
	gl::Texture		mLoadingTex;
	gl::Texture		mParamsTex;
	gl::Texture		mAtmosphereTex;
	gl::Texture		mStarTex, mStarAlternateTex;
	gl::Texture		mStarGlowTex;
	gl::Texture		mSkyDome;
	gl::Texture		mDottedTex;
	gl::Texture		mPanelUpTex, mPanelDownTex;
	gl::Texture		mPlayTex, mPauseTex, mForwardTex, mBackwardTex, mDebugTex, mDebugOnTex;
	vector<gl::Texture*> mPlanetsTex;
	vector<gl::Texture*> mCloudsTex;
	
	float			mTime;
	
	bool			mIsLoaded;
};

void KeplerApp::setup()
{
	mIsLoaded = false;
	
	Rand::randomize();
	
	// INIT ACCELEROMETER
	enableAccelerometer();
	mAccelMatrix		= Matrix44f();
	
	// ARCBALL
	mMatrix	= Quatf();
	mArcball.setWindowSize( getWindowSize() );
	mArcball.setCenter( getWindowCenter() );
	mArcball.setRadius( 420 );
	
	// AUDIO
//	mCurrentTrackId         = 1;
//	mCurrentTrackPlayheadTime = 0;
	// TODO: what about? something like this:
	//mCurrentTrack = mIpodPlayer.getCurrentTrack();
	
	// CAMERA PERSP
	mCamDist			= G_INIT_CAM_DIST;
	mCamDistDest		= mCamDist;
	mCamDistFrom		= mCamDist;
	mEye				= Vec3f( 0.0f, 0.0f, mCamDist );
	mCenter				= Vec3f::zero();
	mCenterDest			= mCenter;
	mCenterFrom			= mCenter;
	mUp					= Vec3f::yAxis();
	mFov				= 90.0f;
	mFovDest			= 90.0f;
	mCam.setPerspective( mFov, getWindowAspectRatio(), 0.001f, 4000.0f );
	mBbRight			= Vec3f::xAxis();
	mBbUp				= Vec3f::yAxis();
	
	// FONTS
	mFont				= Font( loadResource( "UnitRoundedOT-Ultra.otf" ), 16 );
	mFontBig			= Font( loadResource( "UnitRoundedOT-Ultra.otf" ), 256 );
	
	// PLAYER
	mIpodPlayer.registerStateChanged( this, &KeplerApp::onPlayerStateChanged );
    mIpodPlayer.registerTrackChanged( this, &KeplerApp::onPlayerTrackChanged );
	
	
	// TOUCH VARS
	mTouchPos			= Vec2f::zero();
	mTouchThrowVel		= Vec2f::zero();
	mTouchVel			= Vec2f::zero();
	mIsDragging			= false;
	mIsPinching			= false;
	mTime				= getElapsedSeconds();
	mPinchRecognizer.init(this);
    mPinchRecognizer.registerBegan( this, &KeplerApp::onPinchBegan );
    mPinchRecognizer.registerMoved( this, &KeplerApp::onPinchMoved );
    mPinchRecognizer.registerEnded( this, &KeplerApp::onPinchEnded );
	
	
	// TEXTURES
	initTextures();
	
	// BREADCRUMBS
	mBreadcrumbs.setup( this, mFont );
	mBreadcrumbs.registerBreadcrumbSelected( this, &KeplerApp::onBreadcrumbSelected );
	mBreadcrumbs.setHierarchy(mState.getHierarchy());
	
	// PLAY CONTROLS
	mPlayControls.setup( this, mIpodPlayer.getPlayState() == ipod::Player::StatePlaying );
	mPlayControls.registerButtonPressed( this, &KeplerApp::onPlayControlsButtonPressed );
	
	// STATE
	mState.registerAlphaCharStateChanged( this, &KeplerApp::onAlphaCharStateChanged );
	mState.registerNodeSelected( this, &KeplerApp::onNodeSelected );
	
	// UILAYER
	mUiLayer.setup( this );
	mUiLayer.registerAlphaCharSelected( this, &KeplerApp::onAlphaCharSelected );
	mUiLayer.registerWheelClosed( this, &KeplerApp::onWheelClosed );
	mUiLayer.initAlphaTextures( mFontBig );
}

void KeplerApp::init()
{
	// DATA
	mData.initArtists();
	

	// WORLD
	mWorld.setData( &mData );
	mWorld.initNodes( &mIpodPlayer, mFont );
	
	mIsLoaded = true;
}

void KeplerApp::touchesBegan( TouchEvent event )
{	
	mIsDragging = false;
	for( vector<TouchEvent::Touch>::const_iterator touchIt = event.getTouches().begin(); touchIt != event.getTouches().end(); ++touchIt ) 
	{
		mTouchPos		= touchIt->getPos();
		mTouchThrowVel	= Vec2f::zero();
		mIsDragging		= false;
		if( event.getTouches().size() == 1 )
			mArcball.mouseDown( Vec2f( mTouchPos.x, mTouchPos.y ) );
	}
}

void KeplerApp::touchesMoved( TouchEvent event )
{
	mIsDragging = true;
	for( vector<TouchEvent::Touch>::const_iterator touchIt = event.getTouches().begin(); touchIt != event.getTouches().end(); ++touchIt )
	{
		if( event.getTouches().size() == 1 ){
			mTouchThrowVel	= ( touchIt->getPos() - mTouchPos );
			mTouchVel		= mTouchThrowVel;
			mTouchPos		= touchIt->getPos();
			if( event.getTouches().size() == 1 )
				mArcball.mouseDrag( Vec2f( mTouchPos.x, mTouchPos.y ) );
		}
	}
}

void KeplerApp::touchesEnded( TouchEvent event )
{
	for( vector<TouchEvent::Touch>::const_iterator touchIt = event.getTouches().begin(); touchIt != event.getTouches().end(); ++touchIt )
	{
		if( event.getTouches().size() == 1 ){
			mTouchPos = touchIt->getPos();
			if( ! mUiLayer.getShowWheel() && ! mIsDragging ){
				float u			= mTouchPos.x / (float) getWindowWidth();
				float v			= mTouchPos.y / (float) getWindowHeight();
				Ray touchRay	= mCam.generateRay( u, 1.0f - v, mCam.getAspectRatio() );
				checkForNodeTouch( touchRay, mMatrix );
			}
			mIsDragging = false;
		}
	}
}

bool KeplerApp::onPinchBegan( PinchEvent event )
{
	mIsPinching = true;
    mPinchRays = event.getTouchRays( mCam );
	
	mTouchThrowVel	= Vec2f::zero();
	vector<TouchEvent::Touch> touches = event.getTouches();
	Vec2f averageTouchPos;
	for( vector<TouchEvent::Touch>::iterator it = touches.begin(); it != touches.end(); ++it ){
		averageTouchPos += it->getPos();
	}
	averageTouchPos /= touches.size();
	mTouchPos = averageTouchPos;
	
	mArcball.mouseDown( mTouchPos );
    return false;
}

bool KeplerApp::onPinchMoved( PinchEvent event )
{
    mPinchRays = event.getTouchRays( mCam );
	mFovDest += ( 1.0f - event.getScaleDelta() ) * 50.0f;
	
	vector<TouchEvent::Touch> touches = event.getTouches();
	Vec2f averageTouchPos;
	for( vector<TouchEvent::Touch>::iterator it = touches.begin(); it != touches.end(); ++it ){
		averageTouchPos += it->getPos();
	}
	averageTouchPos /= touches.size();
	
	mTouchThrowVel	= ( averageTouchPos - mTouchPos );
	mTouchVel		= mTouchThrowVel;
	mTouchPos		= averageTouchPos;
	
	mArcball.mouseDrag( averageTouchPos );
    return false;
}

bool KeplerApp::onPinchEnded( PinchEvent event )
{
	mIsPinching = false;
    mPinchRays.clear();
    return false;
}

void KeplerApp::accelerated( AccelEvent event )
{
	mNewAccelMatrix = event.getMatrix();
	mNewAccelMatrix.invert();
}

void KeplerApp::initTextures()
{
	mLoadingTex			= loadImage( loadResource( "loading.jpg" ) );
	mPanelUpTex			= loadImage( loadResource( "panelUp.png" ) );
	mPanelDownTex		= loadImage( loadResource( "panelDown.png" ) );
	mPlayTex			= loadImage( loadResource( "play.png" ) );
	mPauseTex			= loadImage( loadResource( "pause.png" ) );
	mForwardTex			= loadImage( loadResource( "forward.png" ) );
	mBackwardTex		= loadImage( loadResource( "backward.png" ) );
	mDebugTex			= loadImage( loadResource( "debug.png" ) );
	mDebugOnTex			= loadImage( loadResource( "debugOn.png" ) );
	mAtmosphereTex		= loadImage( loadResource( "atmosphere.png" ) );
	mStarTex			= loadImage( loadResource( "star.png" ) );
	mStarAlternateTex	= loadImage( loadResource( "starAlternate.png" ) );
	mStarGlowTex		= loadImage( loadResource( "starGlow.png" ) );
	mSkyDome			= loadImage( loadResource( "skydome.jpg" ) );
	mDottedTex			= loadImage( loadResource( "dotted.png" ) );
	mDottedTex.setWrap( GL_REPEAT, GL_REPEAT );
	mParamsTex			= gl::Texture( 768, 75 );
	mPlanetsTex.push_back( new gl::Texture( loadImage( loadResource( "11.jpg" ) ) ) );
	mPlanetsTex.push_back( new gl::Texture( loadImage( loadResource( "21.jpg" ) ) ) );
	mPlanetsTex.push_back( new gl::Texture( loadImage( loadResource( "31.jpg" ) ) ) );
	mPlanetsTex.push_back( new gl::Texture( loadImage( loadResource( "41.jpg" ) ) ) );
	mPlanetsTex.push_back( new gl::Texture( loadImage( loadResource( "51.jpg" ) ) ) );
	mPlanetsTex.push_back( new gl::Texture( loadImage( loadResource( "rings.png" ) ) ) );
	mCloudsTex.push_back( new gl::Texture( loadImage( loadResource( "clouds1.png" ) ) ) );
	mCloudsTex.push_back( new gl::Texture( loadImage( loadResource( "clouds2.png" ) ) ) );
	mCloudsTex.push_back( new gl::Texture( loadImage( loadResource( "clouds3.png" ) ) ) );
	mCloudsTex.push_back( new gl::Texture( loadImage( loadResource( "clouds4.png" ) ) ) );
	mCloudsTex.push_back( new gl::Texture( loadImage( loadResource( "clouds5.png" ) ) ) );
	
}

bool KeplerApp::onWheelClosed( UiLayer *uiLayer )
{
	std::cout << "wheel closed" << std::endl;
	mFovDest = 100.0f;
	return false;
}

bool KeplerApp::onAlphaCharSelected( UiLayer *uiLayer )
{
	mState.setAlphaChar( uiLayer->getAlphaChar() );
	return false;
}

bool KeplerApp::onAlphaCharStateChanged( State *state )
{
	mData.filterArtistsByAlpha( mState.getAlphaChar() );
	mWorld.filterNodes();
	mBreadcrumbs.setHierarchy( mState.getHierarchy() );	
	return false;
}

bool KeplerApp::onNodeSelected( Node *node )
{
	mTime			= getElapsedSeconds();
	mCenterFrom		= mCenter;
	mCamDistFrom	= mCamDist;	
	mZoomFrom		= G_ZOOM;			
	mBreadcrumbs.setHierarchy( mState.getHierarchy() );	

	if( node != NULL && node->mGen == G_TRACK_LEVEL ){
		mIpodPlayer.play( node->mAlbum, node->mIndex );
	}
//	if( node ){
//		if( node->mGen == G_ALBUM_LEVEL ){
//			std::cout << "setting currentAlbum = " << node->mAlbum->getAlbumTitle() << std::endl;
//			mCurrentAlbum = node->mAlbum;
//		} else if( node->mGen == G_TRACK_LEVEL ){
//			std::cout << "setting currentTrackId = " << node->mIndex << std::endl;
//			mCurrentTrackId = node->mIndex;
//		}
//	}
	
	return false;
}

bool KeplerApp::onPlayControlsButtonPressed( PlayControls::PlayButton button )
{
	if( button == PlayControls::PREVIOUS_TRACK ){
		mIpodPlayer.skipPrev();
	} else if( button == PlayControls::PLAY_PAUSE ){
		if (mIpodPlayer.getPlayState() == ipod::Player::StatePlaying) {
			mIpodPlayer.pause();
		}
		else {
			mIpodPlayer.play();
		}
	} else if( button == PlayControls::NEXT_TRACK ){
		mIpodPlayer.skipNext();	
	} else if( button == PlayControls::DEBUG ){
		G_DEBUG = !G_DEBUG;
	}
	cout << "play button " << button << " pressed" << endl;
	return false;
}

bool KeplerApp::onBreadcrumbSelected( BreadcrumbEvent event )
{
	int level = event.getLevel();
	if( level == G_HOME_LEVEL ){				// BACK TO HOME
		mUiLayer.setShowWheel( !mUiLayer.getShowWheel() );
		mWorld.deselectAllNodes();
		mState.setSelectedNode( NULL );
		mState.setAlphaChar( ' ' );
	}
	else if( level == G_ALPHA_LEVEL ){			// BACK TO ALPHA FILTER
		mWorld.deselectAllNodes();
		mData.filterArtistsByAlpha( mState.getAlphaChar() );
		mWorld.filterNodes();
		mState.setSelectedNode(NULL);
	}
	else if( level >= G_ARTIST_LEVEL ){			// BACK TO ARTIST/ALBUM/TRACK
		// get Artist, Album or Track from selectedNode
		Node *current = mState.getSelectedNode();
		while (current != NULL && current->mGen > level) {
			current = current->mParentNode;
		}
		mState.setSelectedNode(current);
	}
	return false;
}

void KeplerApp::checkForNodeTouch( const Ray &ray, Matrix44f &mat )
{
	vector<Node*> nodes;
	mWorld.checkForSphereIntersect( nodes, ray, mat );

	if( nodes.size() > 0 ){
		Node *nodeWithHighestGen = *nodes.begin();
		int highestGen = nodeWithHighestGen->mGen;
		
		vector<Node*>::iterator it;
		for( it = nodes.begin(); it != nodes.end(); ++it ){
			if( (*it)->mGen > highestGen ){
				highestGen = (*it)->mGen;
				nodeWithHighestGen = *it;
			}
		}
		
		
		if( nodeWithHighestGen ){
			if( highestGen == G_ARTIST_LEVEL ){
				if( ! mState.getSelectedArtistNode() )
					mState.setSelectedNode( nodeWithHighestGen );
			} else {
				mState.setSelectedNode( nodeWithHighestGen );
			}

		}
	}
}

void KeplerApp::update()
{
	mAccelMatrix	= lerp( mAccelMatrix, mNewAccelMatrix, 0.17f );
	updateArcball();
	mWorld.update( mMatrix, mBbRight, mBbUp );
	updateCamera();
	mWorld.updateGraphics( mCam );
	
	mUiLayer.update();
	mBreadcrumbs.update();
	mPlayControls.update();
	
	updatePlayhead();
}

void KeplerApp::updateArcball()
{
	if( mTouchThrowVel.length() > 10.0f && !mIsDragging ){
		if( mTouchVel.length() > 1.0f ){
			//mTouchVel *= 0.99f;
			mArcball.mouseDown( mTouchPos );
			mArcball.mouseDrag( mTouchPos + mTouchVel );
		}
	}
	
	if( G_DEBUG ){
		mMatrix = mAccelMatrix * mArcball.getQuat();
	} else {
		mMatrix = mArcball.getQuat();
	}
	
}


void KeplerApp::updateCamera()
{
	Node* selectedNode = mState.getSelectedNode();
	if( selectedNode ){
		mCamDistDest	= selectedNode->mIdealCameraDist;
		mCenterDest		= mMatrix.transformPointAffine( selectedNode->mPos );
		mZoomDest		= selectedNode->mGen;
		
		mCenterFrom		+= selectedNode->mVel;
		//mCamDistFrom	+= selectedNode->mCamZVel;

	} else {
		mCamDistDest	= G_INIT_CAM_DIST;
		mCenterDest		= mMatrix.transformPointAffine( Vec3f::zero() );

		mZoomDest		= G_HOME_LEVEL;
		if( mState.getAlphaChar() != ' ' ){
			mZoomDest	= G_ALPHA_LEVEL;
		}
	}
	
	// UPDATE FOV
	if( mUiLayer.getShowWheel() ){
		mFovDest = 130.0f;
	}
	
	mFovDest = constrain( mFovDest, 75.0f, 130.0f );
	mFov -= ( mFov - mFovDest ) * 0.4f;
	
	
	if( mFovDest == 130.0f && ! mUiLayer.getShowWheel() && G_ZOOM < G_ARTIST_LEVEL ){
		mUiLayer.setShowWheel( true );
		mWorld.deselectAllNodes();
		mState.setSelectedNode( NULL );
		mState.setAlphaChar( ' ' );
	}
	
	double p		= constrain( getElapsedSeconds()-mTime, 0.0, G_DURATION );
	mCenter			= easeInOutQuad( p, mCenterFrom, mCenterDest - mCenterFrom, G_DURATION );
	mCamDist		= easeInOutQuad( p, mCamDistFrom, mCamDistDest - mCamDistFrom, G_DURATION );
	G_ZOOM			= easeInOutQuad( p, mZoomFrom, mZoomDest - mZoomFrom, G_DURATION );
	
	Vec3f prevEye	= mEye;
	mEye			= Vec3f( mCenter.x, mCenter.y, mCenter.z - mCamDist );
	mCamVel			= mEye - prevEye;
	
	mCam.setPerspective( mFov, getWindowAspectRatio(), 0.0001f, 4000.0f );
	mCam.lookAt( mEye, mCenter, mUp );
	mCam.getBillboardVectors( &mBbRight, &mBbUp );
}

void KeplerApp::updatePlayhead()
{
	if( mIpodPlayer.getPlayState() == ipod::Player::StatePlaying ){
		mCurrentTrackPlayheadTime	= mIpodPlayer.getPlayheadTime();
		ipod::TrackRef playingTrack = mIpodPlayer.getPlayingTrack();
		Node* selectedNode = mState.getSelectedNode();
		
		if( selectedNode != NULL ){
			ipod::TrackRef selectedTrack = selectedNode->mTrack;
			if( selectedTrack != NULL && selectedTrack->getItemId() == playingTrack->getItemId() ){
				/* update orbit line */
			}
		}
	}
}

void KeplerApp::draw()
{
	gl::clear( Color( 0, 0, 0 ), true );
	if( !mIsLoaded ){
		mLoadingTex.enableAndBind();
		gl::setMatricesWindow( getWindowSize() );
		gl::drawSolidRect( getWindowBounds() );

		mLoadingTex.disable();
		
		init();
	} else {
		gl::enableDepthWrite();
		gl::setMatrices( mCam );
		
	// DRAW SKYDOME
		gl::pushModelView();
		gl::rotate( mMatrix );
		gl::color( Color( 1.0f, 1.0f, 1.0f ) );
		mSkyDome.enableAndBind();
		gl::drawSphere( Vec3f::zero(), 2000.0f, 64 );
		gl::popModelView();
		
		
		gl::enableAdditiveBlending();
		
	// STARGLOWS
		mStarGlowTex.enableAndBind();
		mWorld.drawStarGlows();
		mStarGlowTex.disable();
		
	// ORBITS
		gl::enableAdditiveBlending();
		gl::enableDepthRead();
		gl::disableDepthWrite();
		mWorld.drawOrbitRings();
		gl::disableDepthRead();
		
	// STARS
		mStarTex.enableAndBind();
		mWorld.drawStars();
		mStarTex.disable();
		
	// CONSTELLATION
		mDottedTex.enableAndBind();
		mWorld.drawConstellation( mMatrix );
		mDottedTex.disable();
		
	// PLANETS
		Node *albumNode  = mState.getSelectedAlbumNode();
		Node *artistNode = mState.getSelectedArtistNode();
		if( artistNode ){
			gl::enableDepthRead();
			gl::enableDepthWrite();
			gl::disableAlphaBlending();
			
			glEnable( GL_LIGHTING );
			glEnable( GL_COLOR_MATERIAL );
			glShadeModel( GL_SMOOTH );
						
			glMaterialfv( GL_FRONT, GL_AMBIENT, mat_ambient );
			glMaterialfv( GL_FRONT, GL_DIFFUSE, mat_diffuse );
			//glMaterialfv( GL_FRONT_AND_BACK, GL_SPECULAR, mat_specular );
			//glMaterialfv( GL_FRONT_AND_BACK, GL_SHININESS, mat_shininess );
			
			if( albumNode ){
				// LIGHT FROM ALBUM
				glEnable( GL_LIGHT0 );
				Vec3f albumLightPos		= albumNode->mTransPos;
				GLfloat albumLight[]	= { albumLightPos.x, albumLightPos.y, albumLightPos.z, 1.0f };
				Color albumDiffuse		= albumNode->mColor;
				glLightfv( GL_LIGHT0, GL_POSITION, albumLight );
				glLightfv( GL_LIGHT0, GL_DIFFUSE, albumDiffuse );
			//	glLightfv( GL_LIGHT0, GL_SPECULAR, albumDiffuse );
			}
			
			// LIGHT FROM ARTIST
			glEnable( GL_LIGHT1 );
			Vec3f artistLightPos	= artistNode->mTransPos;
			GLfloat artistLight[]	= { artistLightPos.x, artistLightPos.y, artistLightPos.z, 1.0f };
			Color artistDiffuse		= artistNode->mColor;
			glLightfv( GL_LIGHT1, GL_POSITION, artistLight );
			glLightfv( GL_LIGHT1, GL_DIFFUSE, artistDiffuse );
			//glLightfv( GL_LIGHT1, GL_SPECULAR, artistDiffuse );
				
	// PLANETS
			mWorld.drawPlanets( mAccelMatrix, mPlanetsTex );
		
	// CLOUDS
			mWorld.drawClouds( mAccelMatrix, mCloudsTex );
			
	// RINGS
			gl::enableAdditiveBlending();
			mWorld.drawRings( mPlanetsTex[G_RING_TYPE] );
			glDisable( GL_LIGHTING );
			gl::disableDepthRead();
		}
		
	// ATMOSPHERE
		mAtmosphereTex.enableAndBind();
		gl::disableDepthRead();
		gl::disableDepthWrite();
		mWorld.drawAtmospheres();
		mAtmosphereTex.disable();


	// NAMES
		gl::disableDepthWrite();
		glEnable( GL_TEXTURE_2D );
		gl::setMatricesWindow( getWindowSize() );
		gl::enableAdditiveBlending();
		mWorld.drawOrthoNames( mCam );
		
		
		
		
		glDisable( GL_TEXTURE_2D );
		gl::disableAlphaBlending();
		gl::enableAlphaBlending();
		mUiLayer.draw( mPanelUpTex, mPanelDownTex );
		mBreadcrumbs.draw( mUiLayer.getPanelYPos() + 5.0f );
		mPlayControls.draw( mPlayTex, mPauseTex, mForwardTex, mBackwardTex, mDebugTex, mDebugOnTex, mUiLayer.getPanelYPos() + mBreadcrumbs.getHeight() + 10.0f );
		mState.draw( mFont );
		
		//drawInfoPanel();
	}
}



void KeplerApp::drawInfoPanel()
{
	gl::setMatricesWindow( getWindowSize() );
	if( getElapsedFrames() % 30 == 0 ){
		setParamsTex();
	}
	gl::color( Color( 1.0f, 1.0f, 1.0f ) );
	gl::draw( mParamsTex, Vec2f( 0.0f, 0.0f ) );
}


void KeplerApp::setParamsTex()
{
	TextLayout layout;	
	layout.setFont( mFont );
	layout.setColor( Color( 0.3f, 0.3f, 1.0f ) );

	int currentLevel = G_HOME_LEVEL;
	if (mState.getSelectedNode()) {
		currentLevel = mState.getSelectedNode()->mGen;
	}
	else if (mState.getAlphaChar() != ' ') {
		currentLevel = G_ALPHA_LEVEL;
	}
	
	stringstream s;
	s.str("");
	s << " CURRENT LEVEL: " << currentLevel;
	layout.addLine( s.str() );
	
	s.str("");
	s << " FPS: " << getAverageFps();
	layout.addLine( s.str() );
	
	s.str("");
	s << " ZOOM LEVEL: " << G_ZOOM;
	layout.setColor( Color( 0.0f, 1.0f, 1.0f ) );
	layout.addLine( s.str() );
	
	mParamsTex = gl::Texture( layout.render( true, false ) );
}


bool KeplerApp::onPlayerTrackChanged( ipod::Player *player )
{	
	std::cout << "==================================================================" << std::endl;
	console() << "track changed!" << endl;

	if (player->getPlayState() == ipod::Player::StatePlaying) {
		
		// get refs to the currently playing things...
		ipod::TrackRef playingTrack = player->getPlayingTrack();
		ipod::PlaylistRef playingArtist = ipod::getArtist(playingTrack->getArtistId());
		ipod::PlaylistRef playingAlbum = ipod::getAlbum(playingTrack->getAlbumId());

		console() << "searching all our nodes for the new playing song..." << endl;
		for (int i = 0; i < mWorld.mNodes.size(); i++) {
			Node *artistNode = mWorld.mNodes[i];
			if (artistNode->mName == playingArtist->getArtistName()) {
				console() << "hey it's an artist we know!" << endl;
				if (!artistNode->mIsSelected) {
					console() << "and it needs to be selected!" << endl;
					mState.setAlphaChar(artistNode->mName);
					mState.setSelectedNode(artistNode);
				}
				for (int j = 0; j < artistNode->mChildNodes.size(); j++) {					
					Node *albumNode = artistNode->mChildNodes[j];
					if (albumNode->mName == playingAlbum->getAlbumTitle()) {
						console() << "and we know the album!" << endl;
						if (!albumNode->mIsSelected) {
							console() << "and the album needs to be selected" << endl;
							mState.setSelectedNode(albumNode);
						}
						for (int k = 0; k < albumNode->mChildNodes.size(); k++) {
							Node *trackNode = albumNode->mChildNodes[k];
							if (trackNode->mTrack->getItemId() == playingTrack->getItemId()) {
								console() << "and we know the track!" << endl;
								if (!trackNode->mIsSelected) {
									console() << "quick! select it!!!" << endl;
									mState.setSelectedNode(trackNode);
								}
								// FIXME: what's the proper C++ way to do this?
								NodeTrack *scaryCastNode = (NodeTrack*)trackNode;
								mState.setPlayingNode(scaryCastNode);
								break;
							}
						}								
						break;
					}
//					else {
//						console() << "new album from current artist, do something clever!" << endl;
//					}
				}
				break;
			}
		}
	}
	else {
		// FIXME: when we pause we'll stop drawing orbits because of this, which is probably the wrong behavior
		mState.setPlayingNode(NULL);
		console() << "trackchanged but nothing's playing" << endl;
	}
	
	/*
	if( mCurrentAlbum ){
		ipod::TrackRef currentTrack = mCurrentAlbum[mCurrentTrackId];
		
		if( player->getPlayingTrack() != currentTrack ){
			cout << "player album ID = " << player->getPlayingTrack()->getAlbumId() << endl;
			cout << "player artist ID = " << player->getPlayingTrack()->getArtistId() << endl;
			
			if( currentTrack ){
				cout << "global album ID = " << currentTrack->getAlbumId() << endl;
				cout << "global artist ID = " << currentTrack->getArtistId() << endl;
			}
			mState.setSelectedNode( currentAlbumNode->mChildNodes[mCurrentTrackId] );
		}
	}*/
	
    //console() << "Now Playing: " << player->getPlayingTrack()->getTitle() << endl;
    return false;
}

bool KeplerApp::onPlayerStateChanged( ipod::Player *player )
{	
	std::cout << "onPlayerStateChanged()" << std::endl;
    switch( player->getPlayState() ){
        case ipod::Player::StatePlaying:
            console() << "Playing..." << endl;
			mPlayControls.setPlaying(true);
            break;
        case ipod::Player::StateStopped:
            console() << "Stopped." << endl;
			mPlayControls.setPlaying(false);
			break;
        default:
            console() << "Other!" << endl;
			mPlayControls.setPlaying(false);
            break;
    }
    return false;
}


CINDER_APP_COCOA_TOUCH( KeplerApp, RendererGl )
