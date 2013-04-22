//
//  Head.cpp
//  interface
//
//  Created by Philip Rosedale on 9/11/12.
//	adapted by Jeffrey Ventrella 
//  Copyright (c) 2012 Physical, Inc.. All rights reserved.
//

#include <glm/glm.hpp>
#include <vector>
#include <lodepng.h>
#include <SharedUtil.h>
#include "Head.h"
#include "Log.h"
#include <AgentList.h>
#include <AgentTypes.h>
#include <PacketHeaders.h>

using namespace std;

float skinColor[] = {1.0, 0.84, 0.66};
float lightBlue[] = { 0.7, 0.8, 1.0 };
float browColor[] = {210.0/255.0, 105.0/255.0, 30.0/255.0};
float mouthColor[] = {1, 0, 0};

float BrowRollAngle[5] = {0, 15, 30, -30, -15};
float BrowPitchAngle[3] = {-70, -60, -50};
float eyeColor[3] = {1,1,1};

float MouthWidthChoices[3] = {0.5, 0.77, 0.3};

float browWidth = 0.8;
float browThickness = 0.16;

bool usingBigSphereCollisionTest = true;

const float DECAY = 0.1;
const float THRUST_MAG	= 10.0;
const float YAW_MAG		= 300.0;

char iris_texture_file[] = "resources/images/green_eye.png";

vector<unsigned char> iris_texture;
unsigned int iris_texture_width = 512;
unsigned int iris_texture_height = 256;

Head::Head(bool isMine) {
    
    _orientation.setToIdentity();
    
	_velocity           = glm::vec3( 0.0, 0.0, 0.0 );
	_thrust		        = glm::vec3( 0.0, 0.0, 0.0 );
    _rotation           = glm::quat( 0.0f, 0.0f, 0.0f, 0.0f );	
	_closestOtherAvatar = 0;
	_bodyYaw            = -90.0;
	_bodyPitch          = 0.0;
	_bodyRoll           = 0.0;
	_bodyYawDelta       = 0.0;
	_mousePressed       = false;
	_mode               = AVATAR_MODE_STANDING;
    _isMine             = isMine;
    _maxArmLength       = 0.0;
    //_transmitterTimer   = 0;
    _transmitterHz      = 0.0;
    _transmitterPackets = 0;
    _numOtherAvatarsInView = 0;

    initializeSkeleton();
    
    _TEST_bigSphereRadius = 0.3f;
    _TEST_bigSpherePosition = glm::vec3( 0.0f, _TEST_bigSphereRadius, 2.0f );
    
    for (int i = 0; i < MAX_DRIVE_KEYS; i++) _driveKeys[i] = false; 
    
    _head.pupilSize          = 0.10;
    _head.interPupilDistance = 0.6;
    _head.interBrowDistance  = 0.75;
    _head.nominalPupilSize   = 0.10;
    _head.yaw                = 0.0;
    _head.pitch              = 0.0;
    _head.roll               = 0.0;
    _head.pitchRate          = 0.0;
    _head.yawRate            = 0.0;
    _head.rollRate           = 0.0;
    _head.eyebrowPitch[0]    = -30;
    _head.eyebrowPitch[1]    = -30;
    _head.eyebrowRoll [0]    = 20;
    _head.eyebrowRoll [1]    = -20;
    _head.mouthPitch         = 0;
    _head.mouthYaw           = 0;
    _head.mouthWidth         = 1.0;
    _head.mouthHeight        = 0.2;
    _head.eyeballPitch[0]    = 0;
    _head.eyeballPitch[1]    = 0;
    _head.eyeballScaleX      = 1.2;  
    _head.eyeballScaleY      = 1.5; 
    _head.eyeballScaleZ      = 1.0;
    _head.eyeballYaw[0]      = 0;
    _head.eyeballYaw[1]      = 0;
    _head.pitchTarget        = 0;
    _head.yawTarget          = 0; 
    _head.noiseEnvelope      = 1.0;
    _head.pupilConverge      = 10.0;
    _head.leanForward        = 0.0;
    _head.leanSideways       = 0.0;
    _head.eyeContact         = 1;
    _head.eyeContactTarget   = LEFT_EYE;
    _head.scale              = 1.0;
    _head.audioAttack        = 0.0;
    _head.loudness           = 0.0;
    _head.averageLoudness    = 0.0;
    _head.lastLoudness       = 0.0;
    _head.browAudioLift      = 0.0;
    _head.noise              = 0;
    
	_movedHandOffset         = glm::vec3( 0.0, 0.0, 0.0 );
    _usingBodySprings        = true;
	_springForce             = 6.0f;
	_springVelocityDecay     = 16.0f;
    _renderYaw               = 0.0;
    _renderPitch             = 0.0;
	
	_sphere = NULL;
	
    if (iris_texture.size() == 0) {
        switchToResourcesParentIfRequired();
        unsigned error = lodepng::decode(iris_texture, iris_texture_width, iris_texture_height, iris_texture_file);
        if (error != 0) {
            printLog("error %u: %s\n", error, lodepng_error_text(error));
        }
    }
		
    //--------------------------------------------------
    // test... just slam them into random positions...
    //--------------------------------------------------
    _otherAvatarHandPosition[ 0 ] = glm::vec3(  0.0f, 0.3f,  2.0f );
    _otherAvatarHandPosition[ 1 ] = glm::vec3(  4.0f, 0.3f,  2.0f );
    _otherAvatarHandPosition[ 2 ] = glm::vec3(  2.0f, 0.3f,  2.0f );
    _otherAvatarHandPosition[ 3 ] = glm::vec3(  1.0f, 0.3f, -4.0f );
    _otherAvatarHandPosition[ 4 ] = glm::vec3( -2.0f, 0.3f, -2.0f );
}

Head::Head(const Head &otherAvatar) {
 
    _velocity               = otherAvatar._velocity;
	_thrust                 = otherAvatar._thrust;
    _rotation               = otherAvatar._rotation;
	_closestOtherAvatar     = otherAvatar._closestOtherAvatar;
	_bodyYaw                = otherAvatar._bodyYaw;
	_bodyPitch              = otherAvatar._bodyPitch;
	_bodyRoll               = otherAvatar._bodyRoll;
	_bodyYawDelta           = otherAvatar._bodyYawDelta;
	_mousePressed       = otherAvatar._mousePressed;
	_mode                   = otherAvatar._mode;
    _isMine                 = otherAvatar._isMine;
    _renderYaw              = otherAvatar._renderYaw;
    _renderPitch            = otherAvatar._renderPitch;
    _maxArmLength           = otherAvatar._maxArmLength;
    _transmitterTimer       = otherAvatar._transmitterTimer;
    _transmitterHz          = otherAvatar._transmitterHz;
    _transmitterPackets     = otherAvatar._transmitterPackets;
    _TEST_bigSphereRadius   = otherAvatar._TEST_bigSphereRadius;
    _TEST_bigSpherePosition = otherAvatar._TEST_bigSpherePosition;
	_movedHandOffset        = otherAvatar._movedHandOffset;
	_usingBodySprings       = otherAvatar._usingBodySprings;
	_springForce            = otherAvatar._springForce;
	_springVelocityDecay    = otherAvatar._springVelocityDecay;    
	_orientation.set( otherAvatar._orientation );
        
	_sphere = NULL;

    initializeSkeleton();
    
    for (int i = 0; i < MAX_DRIVE_KEYS; i++) _driveKeys[i] = otherAvatar._driveKeys[i];

    _head.pupilSize          = otherAvatar._head.pupilSize;
    _head.interPupilDistance = otherAvatar._head.interPupilDistance;
    _head.interBrowDistance  = otherAvatar._head.interBrowDistance;
    _head.nominalPupilSize   = otherAvatar._head.nominalPupilSize;
    _head.yaw                = otherAvatar._head.yaw;
    _head.pitch              = otherAvatar._head.pitch;
    _head.roll               = otherAvatar._head.roll;
    _head.yawRate            = otherAvatar._head.yawRate;
    _head.pitchRate          = otherAvatar._head.pitchRate;
    _head.rollRate           = otherAvatar._head.rollRate;
    _head.eyebrowPitch[0]    = otherAvatar._head.eyebrowPitch[0];
    _head.eyebrowPitch[1]    = otherAvatar._head.eyebrowPitch[1];
    _head.eyebrowRoll [0]    = otherAvatar._head.eyebrowRoll [0];
    _head.eyebrowRoll [1]    = otherAvatar._head.eyebrowRoll [1];
    _head.mouthPitch         = otherAvatar._head.mouthPitch;
    _head.mouthYaw           = otherAvatar._head.mouthYaw;
    _head.mouthWidth         = otherAvatar._head.mouthWidth;
    _head.mouthHeight        = otherAvatar._head.mouthHeight;
    _head.eyeballPitch[0]    = otherAvatar._head.eyeballPitch[0];
    _head.eyeballPitch[1]    = otherAvatar._head.eyeballPitch[1];
    _head.eyeballScaleX      = otherAvatar._head.eyeballScaleX;
    _head.eyeballScaleY      = otherAvatar._head.eyeballScaleY;
    _head.eyeballScaleZ      = otherAvatar._head.eyeballScaleZ;
    _head.eyeballYaw[0]      = otherAvatar._head.eyeballYaw[0];
    _head.eyeballYaw[1]      = otherAvatar._head.eyeballYaw[1];
    _head.pitchTarget        = otherAvatar._head.pitchTarget;
    _head.yawTarget          = otherAvatar._head.yawTarget;
    _head.noiseEnvelope      = otherAvatar._head.noiseEnvelope;
    _head.pupilConverge      = otherAvatar._head.pupilConverge;
    _head.leanForward        = otherAvatar._head.leanForward;
    _head.leanSideways       = otherAvatar._head.leanSideways;
    _head.eyeContact         = otherAvatar._head.eyeContact;
    _head.eyeContactTarget   = otherAvatar._head.eyeContactTarget;
    _head.scale              = otherAvatar._head.scale;
    _head.audioAttack        = otherAvatar._head.audioAttack;
    _head.loudness           = otherAvatar._head.loudness;
    _head.averageLoudness    = otherAvatar._head.averageLoudness;
    _head.lastLoudness       = otherAvatar._head.lastLoudness;
    _head.browAudioLift      = otherAvatar._head.browAudioLift;
    _head.noise              = otherAvatar._head.noise;
    
	
    if (iris_texture.size() == 0) {
        switchToResourcesParentIfRequired();
        unsigned error = lodepng::decode(iris_texture, iris_texture_width, iris_texture_height, iris_texture_file);
        if (error != 0) {
            printLog("error %u: %s\n", error, lodepng_error_text(error));
        }
    }
}

Head::~Head()  {
    if (_sphere != NULL) {
        gluDeleteQuadric(_sphere);
    }
}

Head* Head::clone() const {
    return new Head(*this);
}

void Head::reset() {
    _head.pitch = _head.yaw = _head.roll = 0;
    _head.leanForward = _head.leanSideways = 0;
}


//this pertains to moving the head with the glasses 
//---------------------------------------------------
void Head::UpdateGyros(float frametime, SerialInterface * serialInterface, int head_mirror, glm::vec3 * gravity)
//  Using serial data, update avatar/render position and angles
{
    const float PITCH_ACCEL_COUPLING = 0.5;
    const float ROLL_ACCEL_COUPLING = -1.0;
    float measured_pitch_rate = serialInterface->getRelativeValue(HEAD_PITCH_RATE);
    _head.yawRate = serialInterface->getRelativeValue(HEAD_YAW_RATE);
    float measured_lateral_accel = serialInterface->getRelativeValue(ACCEL_X) -
                                ROLL_ACCEL_COUPLING*serialInterface->getRelativeValue(HEAD_ROLL_RATE);
    float measured_fwd_accel = serialInterface->getRelativeValue(ACCEL_Z) -
                                PITCH_ACCEL_COUPLING*serialInterface->getRelativeValue(HEAD_PITCH_RATE);
    float measured_roll_rate = serialInterface->getRelativeValue(HEAD_ROLL_RATE);
   
    //printLog("Pitch Rate: %d ACCEL_Z: %d\n", serialInterface->getRelativeValue(PITCH_RATE), 
    //                                         serialInterface->getRelativeValue(ACCEL_Z));
    //printLog("Pitch Rate: %d ACCEL_X: %d\n", serialInterface->getRelativeValue(PITCH_RATE), 
    //                                         serialInterface->getRelativeValue(ACCEL_Z));
    //printLog("Pitch: %f\n", Pitch);
    
    //  Update avatar head position based on measured gyro rates
    const float HEAD_ROTATION_SCALE = 0.70;
    const float HEAD_ROLL_SCALE = 0.40;
    const float HEAD_LEAN_SCALE = 0.01;
    const float MAX_PITCH = 45;
    const float MIN_PITCH = -45;
    const float MAX_YAW = 85;
    const float MIN_YAW = -85;

    if ((_head.pitch < MAX_PITCH) && (_head.pitch > MIN_PITCH))
        addPitch(measured_pitch_rate * -HEAD_ROTATION_SCALE * frametime);
    
    addRoll(-measured_roll_rate * HEAD_ROLL_SCALE * frametime);

    if (head_mirror) {
        if ((_head.yaw < MAX_YAW) && (_head.yaw > MIN_YAW))
            addYaw(-_head.yawRate * HEAD_ROTATION_SCALE * frametime);
        addLean(-measured_lateral_accel * frametime * HEAD_LEAN_SCALE, -measured_fwd_accel*frametime * HEAD_LEAN_SCALE);
    } else {
        if ((_head.yaw < MAX_YAW) && (_head.yaw > MIN_YAW))
            addYaw(_head.yawRate * -HEAD_ROTATION_SCALE * frametime);
        addLean(measured_lateral_accel * frametime * -HEAD_LEAN_SCALE, measured_fwd_accel*frametime * HEAD_LEAN_SCALE);        
    } 
}

void Head::addLean(float x, float z) {
    //  Add Body lean as impulse 
    _head.leanSideways += x;
    _head.leanForward  += z;
}


void Head::setLeanForward(float dist){
    _head.leanForward = dist;
}

void Head::setLeanSideways(float dist){
    _head.leanSideways = dist;
}

void Head::setMousePressed( bool d ) {
	_mousePressed = d;
}


void Head::simulate(float deltaTime) {
    
    //-------------------------------------------------------------
    // if the avatar being simulated is mine, then loop through
    // all the other avatars to get information about them...
    //-------------------------------------------------------------
    if ( _isMine )
    {
        //-------------------------------------
        // DEBUG - other avatars...
        //-------------------------------------
        _closestOtherAvatar = -1;
        float closestDistance = 10000.0f;
        
        AgentList * agentList = AgentList::getInstance();
        
        _numOtherAvatarsInView =0;

        for(std::vector<Agent>::iterator agent = agentList->getAgents().begin();
            agent != agentList->getAgents().end();
            agent++) {
            if (( agent->getLinkedData() != NULL && ( agent->getType() == AGENT_TYPE_INTERFACE ) )) {
                Head *otherAvatar = (Head *)agent->getLinkedData();
                
                 if ( _numOtherAvatarsInView < MAX_OTHER_AVATARS ) {
                 
                    //-----------------------------------------------------------
                    // test other avatar hand position for proximity...
                    //-----------------------------------------------------------
                    _otherAvatarHandPosition[ _numOtherAvatarsInView ] = otherAvatar->getBonePosition( AVATAR_BONE_RIGHT_HAND );
                    glm::vec3 v( _bone[ AVATAR_BONE_RIGHT_SHOULDER ].position );
                    v -= _otherAvatarHandPosition[ _numOtherAvatarsInView ];
                    
                    float distance = glm::length( v );
                    if ( distance < _maxArmLength ) {
                        if ( distance < closestDistance ) {
                            closestDistance = distance;
                            _closestOtherAvatar = _numOtherAvatarsInView;
                            _numOtherAvatarsInView++;
                        }
                    }
                }
            }
        }
        
        /*
        ///for testing only (prior to having real avs working)
        for (int o=0; o<NUM_OTHER_AVATARS; o++) {
            //-------------------------------------
            // test other avs for proximity...
            //-------------------------------------
            glm::vec3 v( _bone[ AVATAR_BONE_RIGHT_SHOULDER ].position );
            v -= _DEBUG_otherAvatarListPosition[o];
            
            float distance = glm::length( v );

            if ( distance < _maxArmLength ) {
                if ( distance < closestDistance ) {
                    closestDistance = distance;
                    _closestOtherAvatar = o;
                }
            }
        }
        */
        
    }//if ( _isMine )
        
    if ( usingBigSphereCollisionTest ) {
        //--------------------------------------------------------------
        // test for avatar collision response (using a big sphere :)
        //--------------------------------------------------------------
        updateBigSphereCollisionTest(deltaTime);
    }
            
	//------------------------
	// update avatar skeleton
	//------------------------ 
	updateSkeleton();
	
	//------------------------------------------------------------
	// reset hand and arm positions according to hand movement
	//------------------------------------------------------------
	if (_usingBodySprings) {
		updateHandMovement();
		updateBodySprings( deltaTime );
	}
	
    if ( _isMine ) { // driving the avatar around should only apply if this is my avatar (as opposed to an avatar being driven remotely) 
        //-------------------------------------------------
        // this handles the avatar being driven around...
        //-------------------------------------------------	
        _thrust = glm::vec3( 0.0, 0.0, 0.0 );
             
        if (_driveKeys[FWD]) {
            glm::vec3 front( _orientation.getFront().x, _orientation.getFront().y, _orientation.getFront().z );
            _thrust += front * THRUST_MAG;
        }
        if (_driveKeys[BACK]) {
            glm::vec3 front( _orientation.getFront().x, _orientation.getFront().y, _orientation.getFront().z );
            _thrust -= front * THRUST_MAG;
        }
        if (_driveKeys[RIGHT]) {
            glm::vec3 right( _orientation.getRight().x, _orientation.getRight().y, _orientation.getRight().z );
            _thrust += right * THRUST_MAG;
        }
        if (_driveKeys[LEFT]) {
            glm::vec3 right( _orientation.getRight().x, _orientation.getRight().y, _orientation.getRight().z );
            _thrust -= right * THRUST_MAG;
        }
        if (_driveKeys[UP]) {
            glm::vec3 up( _orientation.getUp().x, _orientation.getUp().y, _orientation.getUp().z );
            _thrust += up * THRUST_MAG;
        }
        if (_driveKeys[DOWN]) {
            glm::vec3 up( _orientation.getUp().x, _orientation.getUp().y, _orientation.getUp().z );
            _thrust -= up * THRUST_MAG;
        }
        if (_driveKeys[ROT_RIGHT]) {	
            _bodyYawDelta -= YAW_MAG * deltaTime;
        }
        if (_driveKeys[ROT_LEFT]) {	
            _bodyYawDelta += YAW_MAG * deltaTime;
        }
	}
    
    
	//----------------------------------------------------------
	float translationalSpeed = glm::length( _velocity );
	float rotationalSpeed = fabs( _bodyYawDelta );
	if ( translationalSpeed + rotationalSpeed > 0.2 )
	{
		_mode = AVATAR_MODE_WALKING;
	}
	else
	{
		_mode = AVATAR_MODE_INTERACTING;
	}
		
	//----------------------------------------------------------
	// update body yaw by body yaw delta
	//----------------------------------------------------------
    if (_isMine) {
    _bodyYaw += _bodyYawDelta * deltaTime;
    }
        
	// we will be eventually getting head rotation from elsewhere. For now, just setting it to body rotation 
	_head.yaw   = _bodyYaw;
	_head.pitch = _bodyPitch;
	_head.roll  = _bodyRoll;
	
	//----------------------------------------------------------
	// decay body yaw delta
	//----------------------------------------------------------
    const float TEST_YAW_DECAY = 5.0;
    _bodyYawDelta *= (1.0 - TEST_YAW_DECAY * deltaTime);

	//----------------------------------------------------------
	// add thrust to velocity
	//----------------------------------------------------------
	_velocity += glm::dvec3(_thrust * deltaTime);

    //----------------------------------------------------------
    // update position by velocity
    //----------------------------------------------------------
    _bodyPosition += (glm::vec3)_velocity * deltaTime;

	//----------------------------------------------------------
	// decay velocity
	//----------------------------------------------------------
    const float LIN_VEL_DECAY = 5.0;
    _velocity *= ( 1.0 - LIN_VEL_DECAY * deltaTime );
	
    if (!_head.noise) {
        //  Decay back toward center 
        _head.pitch *= (1.0f - DECAY*2*deltaTime);
        _head.yaw   *= (1.0f - DECAY*2*deltaTime);
        _head.roll  *= (1.0f - DECAY*2*deltaTime);
    }
    else {
        //  Move toward new target  
        _head.pitch += (_head.pitchTarget - _head.pitch)*10*deltaTime; // (1.f - DECAY*deltaTime)*Pitch + ;
        _head.yaw   += (_head.yawTarget   - _head.yaw  )*10*deltaTime; //  (1.f - DECAY*deltaTime);
        _head.roll *= (1.f - DECAY*deltaTime);
    }
    
    _head.leanForward  *= (1.f - DECAY*30.f*deltaTime);
    _head.leanSideways *= (1.f - DECAY*30.f*deltaTime);
    
    //  Update where the avatar's eyes are 
    //
    //  First, decide if we are making eye contact or not
    if (randFloat() < 0.005) {
        _head.eyeContact = !_head.eyeContact;
        _head.eyeContact = 1;
        if (!_head.eyeContact) {
            //  If we just stopped making eye contact,move the eyes markedly away
            _head.eyeballPitch[0] = _head.eyeballPitch[1] = _head.eyeballPitch[0] + 5.0 + (randFloat() - 0.5)*10;
            _head.eyeballYaw  [0] = _head.eyeballYaw  [1] = _head.eyeballYaw  [0] + 5.0 + (randFloat() - 0.5)*5;
        } else {
            //  If now making eye contact, turn head to look right at viewer
            SetNewHeadTarget(0,0);
        }
    }
    
    const float DEGREES_BETWEEN_VIEWER_EYES = 3;
    const float DEGREES_TO_VIEWER_MOUTH = 7;

    if (_head.eyeContact) {
        //  Should we pick a new eye contact target?
        if (randFloat() < 0.01) {
            //  Choose where to look next
            if (randFloat() < 0.1) {
                _head.eyeContactTarget = MOUTH;
            } else {
                if (randFloat() < 0.5) _head.eyeContactTarget = LEFT_EYE; else _head.eyeContactTarget = RIGHT_EYE;
            }
        }
        //  Set eyeball pitch and yaw to make contact
        float eye_target_yaw_adjust = 0;
        float eye_target_pitch_adjust = 0;
        if (_head.eyeContactTarget == LEFT_EYE) eye_target_yaw_adjust = DEGREES_BETWEEN_VIEWER_EYES;
        if (_head.eyeContactTarget == RIGHT_EYE) eye_target_yaw_adjust = -DEGREES_BETWEEN_VIEWER_EYES;
        if (_head.eyeContactTarget == MOUTH) eye_target_pitch_adjust = DEGREES_TO_VIEWER_MOUTH;
        
        _head.eyeballPitch[0] = _head.eyeballPitch[1] = -_head.pitch + eye_target_pitch_adjust;
        _head.eyeballYaw[0] = _head.eyeballYaw[1] = -_head.yaw + eye_target_yaw_adjust;
    }
	

    if (_head.noise)
    {
        _head.pitch += (randFloat() - 0.5)*0.2*_head.noiseEnvelope;
        _head.yaw += (randFloat() - 0.5)*0.3*_head.noiseEnvelope;
        //PupilSize += (randFloat() - 0.5)*0.001*NoiseEnvelope;
        
        if (randFloat() < 0.005) _head.mouthWidth = MouthWidthChoices[rand()%3];
        
        if (!_head.eyeContact) {
            if (randFloat() < 0.01)  _head.eyeballPitch[0] = _head.eyeballPitch[1] = (randFloat() - 0.5)*20;
            if (randFloat() < 0.01)  _head.eyeballYaw[0] = _head.eyeballYaw[1] = (randFloat()- 0.5)*10;
        }         

        if ((randFloat() < 0.005) && (fabs(_head.pitchTarget - _head.pitch) < 1.0) && (fabs(_head.yawTarget - _head.yaw) < 1.0)) {
            SetNewHeadTarget((randFloat()-0.5)*20.0, (randFloat()-0.5)*45.0);
        }

        if (0) {

            //  Pick new target
            _head.pitchTarget = (randFloat() - 0.5)*45;
            _head.yawTarget = (randFloat() - 0.5)*22;
        }
        if (randFloat() < 0.01)
        {
            _head.eyebrowPitch[0] = _head.eyebrowPitch[1] = BrowPitchAngle[rand()%3];
            _head.eyebrowRoll [0] = _head.eyebrowRoll[1] = BrowRollAngle[rand()%5];
            _head.eyebrowRoll [1]*=-1;
        }
    }
}
      
	  
	  
	  
//--------------------------------------------------------------------------------
// This is a workspace for testing avatar body collision detection and response
//--------------------------------------------------------------------------------
void Head::updateBigSphereCollisionTest( float deltaTime ) {

    float myBodyApproximateBoundingRadius = 1.0f;
    glm::vec3 vectorFromMyBodyToBigSphere(_bodyPosition - _TEST_bigSpherePosition);
    bool jointCollision = false;

    float distanceToBigSphere = glm::length(vectorFromMyBodyToBigSphere);
    if ( distanceToBigSphere < myBodyApproximateBoundingRadius + _TEST_bigSphereRadius)
    {
        for (int b=0; b<NUM_AVATAR_BONES; b++) 
        {
            glm::vec3 vectorFromJointToBigSphereCenter(_bone[b].position - _TEST_bigSpherePosition);
            float distanceToBigSphereCenter = glm::length(vectorFromJointToBigSphereCenter);
            float combinedRadius = _bone[b].radius + _TEST_bigSphereRadius;
            if ( distanceToBigSphereCenter < combinedRadius ) 
            {
                jointCollision = true;
                if (distanceToBigSphereCenter > 0.0) 
                {
                    glm::vec3 directionVector = vectorFromJointToBigSphereCenter / distanceToBigSphereCenter;
                    
                    float penetration = 1.0 - (distanceToBigSphereCenter / combinedRadius); 
                    glm::vec3 collisionForce = vectorFromJointToBigSphereCenter * penetration;  
                                          
                    _bone[b].springyVelocity += collisionForce *  8.0f * deltaTime;
                    _velocity                += collisionForce * 18.0f * deltaTime;
                    _bone[b].position = _TEST_bigSpherePosition + directionVector * combinedRadius;
                }
            }
        }
        
        if ( jointCollision ) {
            if (!_usingBodySprings) {
                _usingBodySprings = true;
                initializeBodySprings();
            }
            
            //----------------------------------------------------------
            // add gravity to velocity
            //----------------------------------------------------------
            _velocity += glm::dvec3( 0.0, -1.0, 0.0 ) * 0.05;
            
            //----------------------------------------------------------
            // ground collisions
            //----------------------------------------------------------
            if ( _bodyPosition.y < 0.0 ) {
                _bodyPosition.y = 0.0;
                if ( _velocity.y < 0.0 ) {
                    _velocity.y *= -0.7;
                }
            }       
        }     
    }
}
      
      
      
	   
void Head::render(int faceToFace) {

	//---------------------------------------------------
	// show avatar position
	//---------------------------------------------------
    glColor4f( 0.5f, 0.5f, 0.5f, 0.6 );
	glPushMatrix();
		glTranslatef(_bodyPosition.x, _bodyPosition.y, _bodyPosition.z);
		glScalef( 0.03, 0.03, 0.03 );
        glutSolidSphere( 1, 10, 10 );
	glPopMatrix();
    
    
    if ( usingBigSphereCollisionTest ) {
        //---------------------------------------------------
        // show TEST big sphere 
        //---------------------------------------------------
        glColor4f( 0.5f, 0.6f, 0.8f, 0.7 );
        glPushMatrix();
            glTranslatef(_TEST_bigSpherePosition.x, _TEST_bigSpherePosition.y, _TEST_bigSpherePosition.z);
            glScalef( _TEST_bigSphereRadius, _TEST_bigSphereRadius, _TEST_bigSphereRadius );
            glutSolidSphere( 1, 20, 20 );
        glPopMatrix();
     }
     
	//---------------------------------------------------
	// show avatar orientation
	//---------------------------------------------------
	renderOrientationDirections( _bone[ AVATAR_BONE_HEAD ].position, _bone[ AVATAR_BONE_HEAD ].orientation, 0.2f );
	
	//---------------------------------------------------
	// render body
	//---------------------------------------------------
	renderBody();

	//---------------------------------------------------
	// render head
	//---------------------------------------------------
	renderHead(faceToFace);
	
	//---------------------------------------------------------------------------
	// if this is my avatar, then render my interactions with the other avatars
	//---------------------------------------------------------------------------
    if ( _isMine )
    {
        //---------------------------------------------------
        // render other avatars (DEBUG TEST)
        //---------------------------------------------------
        for (int o=0; o<_numOtherAvatarsInView; o++) {
            glPushMatrix();
                glTranslatef( _otherAvatarHandPosition[o].x, _otherAvatarHandPosition[o].y, _otherAvatarHandPosition[o].z );
                glScalef( 0.03, 0.03, 0.03 );
                glutSolidSphere( 1, 10, 10 );
            glPopMatrix();
        }

        if (_usingBodySprings) {
            if ( _closestOtherAvatar != -1 ) {					

                glm::vec3 v1( _bone[ AVATAR_BONE_RIGHT_HAND ].position );
                glm::vec3 v2( _otherAvatarHandPosition[ _closestOtherAvatar ] );
                
                glLineWidth( 8.0 );
                glColor4f( 0.7f, 0.4f, 0.1f, 0.6 );
                glBegin( GL_LINE_STRIP );
                glVertex3f( v1.x, v1.y, v1.z );
                glVertex3f( v2.x, v2.y, v2.z );
                glEnd();
            }
        }
    }
}

	  
	   
void Head::renderHead(int faceToFace) {
    int side = 0;
        
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_RESCALE_NORMAL);
    
    glPushMatrix();
    
	if (_usingBodySprings) {
		glTranslatef
		( 
			_bone[ AVATAR_BONE_HEAD ].springyPosition.x, 
			_bone[ AVATAR_BONE_HEAD ].springyPosition.y, 
			_bone[ AVATAR_BONE_HEAD ].springyPosition.z 
		);
	}
	else {
		glTranslatef
		( 
			_bone[ AVATAR_BONE_HEAD ].position.x, 
			_bone[ AVATAR_BONE_HEAD ].position.y, 
			_bone[ AVATAR_BONE_HEAD ].position.z 
		);
	}
	
	
	glScalef( 0.03, 0.03, 0.03 );

    glRotatef(_head.yaw,   0, 1, 0); 
    glRotatef(_head.pitch, 1, 0, 0);
    glRotatef(_head.roll,  0, 0, 1);
    
    // Overall scale of head
    if (faceToFace) glScalef(2.0, 2.0, 2.0);
    else glScalef(0.75, 1.0, 1.0);
    

    //  Head
    if (_isMine) {
        glColor3fv(skinColor);
    }
    else {
        glColor3f(0,0,1); //  Temp:  Other people are BLUE
    }
    glutSolidSphere(1, 30, 30);
            
    //  Ears
    glPushMatrix();
        glTranslatef(1.0, 0, 0);
        for(side = 0; side < 2; side++) {
            glPushMatrix();
                glScalef(0.3, 0.65, .65);
                glutSolidSphere(0.5, 30, 30);  
            glPopMatrix();
            glTranslatef(-2.0, 0, 0);
        }
    glPopMatrix();

    // _eyebrows
    _head.audioAttack = 0.9*_head.audioAttack + 0.1*fabs(_head.loudness - _head.lastLoudness);
    _head.lastLoudness = _head.loudness;

    const float BROW_LIFT_THRESHOLD = 100;
    if (_head.audioAttack > BROW_LIFT_THRESHOLD)
        _head.browAudioLift += sqrt(_head.audioAttack)/1000.0;
    
    _head.browAudioLift *= .90;
    
    glPushMatrix();
        glTranslatef(-_head.interBrowDistance/2.0,0.4,0.45);
        for(side = 0; side < 2; side++) {
            glColor3fv(browColor);
            glPushMatrix();
                glTranslatef(0, 0.35 + _head.browAudioLift, 0);
                glRotatef(_head.eyebrowPitch[side]/2.0, 1, 0, 0);
                glRotatef(_head.eyebrowRoll[side]/2.0, 0, 0, 1);
                glScalef(browWidth, browThickness, 1);
                glutSolidCube(0.5);
            glPopMatrix();
            glTranslatef(_head.interBrowDistance, 0, 0);
        }
    glPopMatrix();
    
    
    // Mouth
    
    glPushMatrix();
        glTranslatef(0,-0.35,0.75);
        glColor3f(0,0,0);
        glRotatef(_head.mouthPitch, 1, 0, 0);
        glRotatef(_head.mouthYaw, 0, 0, 1);
        glScalef(_head.mouthWidth*(.7 + sqrt(_head.averageLoudness)/60.0), _head.mouthHeight*(1.0 + sqrt(_head.averageLoudness)/30.0), 1);
        glutSolidCube(0.5);
    glPopMatrix();
    
    glTranslatef(0, 1.0, 0);
   
    glTranslatef(-_head.interPupilDistance/2.0,-0.68,0.7);
    // Right Eye
    glRotatef(-10, 1, 0, 0);
    glColor3fv(eyeColor);
    glPushMatrix(); 
    {
        glTranslatef(_head.interPupilDistance/10.0, 0, 0.05);
        glRotatef(20, 0, 0, 1);
        glScalef(_head.eyeballScaleX, _head.eyeballScaleY, _head.eyeballScaleZ);
        glutSolidSphere(0.25, 30, 30);
    }
    glPopMatrix();
    
    // Right Pupil
    if (_sphere == NULL) {
        _sphere = gluNewQuadric();
        gluQuadricTexture(_sphere, GL_TRUE);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        gluQuadricOrientation(_sphere, GLU_OUTSIDE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, iris_texture_width, iris_texture_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, &iris_texture[0]);
    }

    glPushMatrix();
    {
        glRotatef(_head.eyeballPitch[1], 1, 0, 0);
        glRotatef(_head.eyeballYaw[1] + _head.yaw + _head.pupilConverge, 0, 1, 0);
        glTranslatef(0,0,.35);
        glRotatef(-75,1,0,0);
        glScalef(1.0, 0.4, 1.0);
        
        glEnable(GL_TEXTURE_2D);
        gluSphere(_sphere, _head.pupilSize, 15, 15);
        glDisable(GL_TEXTURE_2D);
    }

    glPopMatrix();
    // Left Eye
    glColor3fv(eyeColor);
    glTranslatef(_head.interPupilDistance, 0, 0);
    glPushMatrix(); 
    {
        glTranslatef(-_head.interPupilDistance/10.0, 0, .05);
        glRotatef(-20, 0, 0, 1);
        glScalef(_head.eyeballScaleX, _head.eyeballScaleY, _head.eyeballScaleZ);
        glutSolidSphere(0.25, 30, 30);
    }
    glPopMatrix();
    // Left Pupil
    glPushMatrix();
    {
        glRotatef(_head.eyeballPitch[0], 1, 0, 0);
        glRotatef(_head.eyeballYaw[0] + _head.yaw - _head.pupilConverge, 0, 1, 0);
        glTranslatef(0, 0, .35);
        glRotatef(-75, 1, 0, 0);
        glScalef(1.0, 0.4, 1.0);

        glEnable(GL_TEXTURE_2D);
        gluSphere(_sphere, _head.pupilSize, 15, 15);
        glDisable(GL_TEXTURE_2D);
    }
    
    glPopMatrix();


    glPopMatrix();
 }
 
void Head::startHandMovement() {

    if (!_usingBodySprings) {
        initializeBodySprings();
        _usingBodySprings = true;
    }
}

void Head::stopHandMovement() {
//_usingBodySprings = false;
}
 
void Head::setHandMovementValues( glm::vec3 handOffset ) {
	_movedHandOffset = handOffset;
}

AvatarMode Head::getMode() {
	return _mode;
}

void Head::initializeSkeleton() {

	for (int b=0; b<NUM_AVATAR_BONES; b++) {
        _bone[b].parent              = AVATAR_BONE_NULL;
        _bone[b].position			 = glm::vec3( 0.0, 0.0, 0.0 );
        _bone[b].defaultPosePosition = glm::vec3( 0.0, 0.0, 0.0 );
        _bone[b].springyPosition     = glm::vec3( 0.0, 0.0, 0.0 );
        _bone[b].springyVelocity     = glm::vec3( 0.0, 0.0, 0.0 );
        _bone[b].rotation            = glm::quat( 0.0f, 0.0f, 0.0f, 0.0f );
        _bone[b].yaw                 = 0.0;
        _bone[b].pitch               = 0.0;
        _bone[b].roll                = 0.0;
        _bone[b].length              = 0.0;
        _bone[b].radius              = 0.02; //default
        _bone[b].springBodyTightness = 4.0;
        _bone[b].orientation.setToIdentity();
	}

	//----------------------------------------------------------------------------
	// parental hierarchy
	//----------------------------------------------------------------------------

	//----------------------------------------------------------------------------
	// spine and head
	//----------------------------------------------------------------------------
	_bone[ AVATAR_BONE_PELVIS_SPINE		].parent = AVATAR_BONE_NULL;
	_bone[ AVATAR_BONE_MID_SPINE        ].parent = AVATAR_BONE_PELVIS_SPINE;
	_bone[ AVATAR_BONE_CHEST_SPINE		].parent = AVATAR_BONE_MID_SPINE;
	_bone[ AVATAR_BONE_NECK				].parent = AVATAR_BONE_CHEST_SPINE;
	_bone[ AVATAR_BONE_HEAD				].parent = AVATAR_BONE_NECK;
	
	//----------------------------------------------------------------------------
	// left chest and arm
	//----------------------------------------------------------------------------
	_bone[ AVATAR_BONE_LEFT_CHEST		].parent = AVATAR_BONE_MID_SPINE;
	_bone[ AVATAR_BONE_LEFT_SHOULDER    ].parent = AVATAR_BONE_LEFT_CHEST;
	_bone[ AVATAR_BONE_LEFT_UPPER_ARM	].parent = AVATAR_BONE_LEFT_SHOULDER;
	_bone[ AVATAR_BONE_LEFT_FOREARM		].parent = AVATAR_BONE_LEFT_UPPER_ARM;
	_bone[ AVATAR_BONE_LEFT_HAND		].parent = AVATAR_BONE_LEFT_FOREARM;

	//----------------------------------------------------------------------------
	// right chest and arm
	//----------------------------------------------------------------------------
	_bone[ AVATAR_BONE_RIGHT_CHEST		].parent = AVATAR_BONE_MID_SPINE;
	_bone[ AVATAR_BONE_RIGHT_SHOULDER	].parent = AVATAR_BONE_RIGHT_CHEST;
	_bone[ AVATAR_BONE_RIGHT_UPPER_ARM	].parent = AVATAR_BONE_RIGHT_SHOULDER;
	_bone[ AVATAR_BONE_RIGHT_FOREARM	].parent = AVATAR_BONE_RIGHT_UPPER_ARM;
	_bone[ AVATAR_BONE_RIGHT_HAND		].parent = AVATAR_BONE_RIGHT_FOREARM;
	
	//----------------------------------------------------------------------------
	// left pelvis and leg
	//----------------------------------------------------------------------------
	_bone[ AVATAR_BONE_LEFT_PELVIS		].parent = AVATAR_BONE_PELVIS_SPINE;
	_bone[ AVATAR_BONE_LEFT_THIGH		].parent = AVATAR_BONE_LEFT_PELVIS;
	_bone[ AVATAR_BONE_LEFT_SHIN		].parent = AVATAR_BONE_LEFT_THIGH;
	_bone[ AVATAR_BONE_LEFT_FOOT		].parent = AVATAR_BONE_LEFT_SHIN;

	//----------------------------------------------------------------------------
	// right pelvis and leg
	//----------------------------------------------------------------------------
	_bone[ AVATAR_BONE_RIGHT_PELVIS		].parent = AVATAR_BONE_PELVIS_SPINE;
	_bone[ AVATAR_BONE_RIGHT_THIGH		].parent = AVATAR_BONE_RIGHT_PELVIS;
	_bone[ AVATAR_BONE_RIGHT_SHIN		].parent = AVATAR_BONE_RIGHT_THIGH;
	_bone[ AVATAR_BONE_RIGHT_FOOT		].parent = AVATAR_BONE_RIGHT_SHIN;

	//----------------------------------------------------------
	// specify the default pose position
	//----------------------------------------------------------
	_bone[ AVATAR_BONE_PELVIS_SPINE		].defaultPosePosition = glm::vec3(  0.0,   0.3,  0.0  );
	_bone[ AVATAR_BONE_MID_SPINE		].defaultPosePosition = glm::vec3(  0.0,   0.1,  0.0  );
	_bone[ AVATAR_BONE_CHEST_SPINE		].defaultPosePosition = glm::vec3(  0.0,   0.1,  0.0  );
	_bone[ AVATAR_BONE_NECK				].defaultPosePosition = glm::vec3(  0.0,   0.06, 0.0  );
	_bone[ AVATAR_BONE_HEAD				].defaultPosePosition = glm::vec3(  0.0,   0.06, 0.0  );
	_bone[ AVATAR_BONE_LEFT_CHEST		].defaultPosePosition = glm::vec3( -0.06,  0.06, 0.0  );
	_bone[ AVATAR_BONE_LEFT_SHOULDER	].defaultPosePosition = glm::vec3( -0.03,  0.0,  0.0  );
	_bone[ AVATAR_BONE_LEFT_UPPER_ARM	].defaultPosePosition = glm::vec3(  0.0,  -0.12, 0.0  );
	_bone[ AVATAR_BONE_LEFT_FOREARM		].defaultPosePosition = glm::vec3(  0.0,  -0.1,  0.0  );
	_bone[ AVATAR_BONE_LEFT_HAND		].defaultPosePosition = glm::vec3(  0.0,  -0.05, 0.0  );
	_bone[ AVATAR_BONE_RIGHT_CHEST		].defaultPosePosition = glm::vec3(  0.06,  0.06, 0.0  );
	_bone[ AVATAR_BONE_RIGHT_SHOULDER	].defaultPosePosition = glm::vec3(  0.03,  0.0,  0.0  );
	_bone[ AVATAR_BONE_RIGHT_UPPER_ARM	].defaultPosePosition = glm::vec3(  0.0,  -0.12, 0.0  );
	_bone[ AVATAR_BONE_RIGHT_FOREARM	].defaultPosePosition = glm::vec3(  0.0,  -0.1,  0.0  );
	_bone[ AVATAR_BONE_RIGHT_HAND		].defaultPosePosition = glm::vec3(  0.0,  -0.05, 0.0  );
	_bone[ AVATAR_BONE_LEFT_PELVIS		].defaultPosePosition = glm::vec3( -0.05,  0.0,  0.0  );
	_bone[ AVATAR_BONE_LEFT_THIGH		].defaultPosePosition = glm::vec3(  0.0,  -0.15, 0.0  );
	_bone[ AVATAR_BONE_LEFT_SHIN		].defaultPosePosition = glm::vec3(  0.0,  -0.15, 0.0  );
	_bone[ AVATAR_BONE_LEFT_FOOT		].defaultPosePosition = glm::vec3(  0.0,   0.0,  0.04 );
	_bone[ AVATAR_BONE_RIGHT_PELVIS		].defaultPosePosition = glm::vec3(  0.05,  0.0,  0.0  );
	_bone[ AVATAR_BONE_RIGHT_THIGH		].defaultPosePosition = glm::vec3(  0.0,  -0.15, 0.0  );
	_bone[ AVATAR_BONE_RIGHT_SHIN		].defaultPosePosition = glm::vec3(  0.0,  -0.15, 0.0  );
	_bone[ AVATAR_BONE_RIGHT_FOOT		].defaultPosePosition = glm::vec3(  0.0,   0.0,  0.04 );

	//----------------------------------------------------------------------------
	// calculate bone length
	//----------------------------------------------------------------------------
	calculateBoneLengths();

	//----------------------------------------------------------------------------
	// generate world positions
	//----------------------------------------------------------------------------
	updateSkeleton();
}




void Head::calculateBoneLengths() {
	for (int b=0; b<NUM_AVATAR_BONES; b++) {
		_bone[b].length = glm::length( _bone[b].defaultPosePosition );
	}

	_maxArmLength
	= _bone[ AVATAR_BONE_RIGHT_UPPER_ARM ].length
	+ _bone[ AVATAR_BONE_RIGHT_FOREARM	 ].length 
	+ _bone[ AVATAR_BONE_RIGHT_HAND		 ].length;
}

void Head::updateSkeleton() {
	//----------------------------------
	// rotate body...
	//----------------------------------	
	_orientation.setToIdentity();
	_orientation.yaw( _bodyYaw );

	//------------------------------------------------------------------------
	// calculate positions of all bones by traversing the skeleton tree:
	//------------------------------------------------------------------------
	for (int b=0; b<NUM_AVATAR_BONES; b++) {	
		if ( _bone[b].parent == AVATAR_BONE_NULL ) {
            _bone[b].orientation.set( _orientation );
			_bone[b].position = _bodyPosition;
		}
		else {
			_bone[b].orientation.set( _bone[ _bone[b].parent ].orientation );
			_bone[b].position = _bone[ _bone[b].parent ].position;
		}
        
        ///TEST! - get this working and then add a comment; JJV
        if ( ! _isMine ) {
            _bone[ AVATAR_BONE_RIGHT_HAND ].position = _handPosition;
        }

		float xx = glm::dot( _bone[b].defaultPosePosition, _bone[b].orientation.getRight() );
		float yy = glm::dot( _bone[b].defaultPosePosition, _bone[b].orientation.getUp	() );
		float zz = glm::dot( _bone[b].defaultPosePosition, _bone[b].orientation.getFront() );

		glm::vec3 rotatedBoneVector( xx, yy, zz );
        
        //glm::vec3 myEuler ( 0.0f, 0.0f, 0.0f );
        //glm::quat myQuat ( myEuler );
        
		_bone[b].position += rotatedBoneVector;
	}	
}


void Head::initializeBodySprings() {
	for (int b=0; b<NUM_AVATAR_BONES; b++) {
		_bone[b].springyPosition = _bone[b].position;
		_bone[b].springyVelocity = glm::vec3( 0.0f, 0.0f, 0.0f );
	}
}


void Head::updateBodySprings( float deltaTime ) {
	for (int b=0; b<NUM_AVATAR_BONES; b++) {
		glm::vec3 springVector( _bone[b].springyPosition );

		if ( _bone[b].parent == AVATAR_BONE_NULL ) {
			springVector -= _bodyPosition;
		}
		else {
			springVector -= _bone[ _bone[b].parent ].springyPosition;
		}

		float length = glm::length( springVector );
		
		if ( length > 0.0f ) {
			glm::vec3 springDirection = springVector / length;
			
			float force = ( length - _bone[b].length ) * _springForce * deltaTime;
			
			_bone[b].springyVelocity -= springDirection * force;
            
            if ( _bone[b].parent != AVATAR_BONE_NULL ) {
                _bone[ _bone[b].parent	].springyVelocity += springDirection * force;
            }
		}
        
		_bone[b].springyVelocity += ( _bone[b].position - _bone[b].springyPosition ) * _bone[b].springBodyTightness * deltaTime;

		float decay = 1.0 - _springVelocityDecay * deltaTime;
		
		if ( decay > 0.0 ) {
			_bone[b].springyVelocity *= decay;
		}
		else {
			_bone[b].springyVelocity = glm::vec3( 0.0f, 0.0f, 0.0f );		
		}

		_bone[b].springyPosition += _bone[b].springyVelocity;
	}
}

glm::vec3 Head::getHeadLookatDirection() {
	return glm::vec3
	(
        _orientation.getFront().x,
        _orientation.getFront().y,
        _orientation.getFront().z
	);
}

glm::vec3 Head::getHeadLookatDirectionUp() {
	return glm::vec3
	(
        _orientation.getUp().x,
        _orientation.getUp().y,
        _orientation.getUp().z
	);
}

glm::vec3 Head::getHeadLookatDirectionRight() {
	return glm::vec3
	(
        _orientation.getRight().x,
        _orientation.getRight().y,
        _orientation.getRight().z
	);
}

glm::vec3 Head::getHeadPosition() {
	return glm::vec3
	(
		_bone[ AVATAR_BONE_HEAD ].position.x,
		_bone[ AVATAR_BONE_HEAD ].position.y,
		_bone[ AVATAR_BONE_HEAD ].position.z
	);
}


glm::vec3 Head::getBonePosition( AvatarBoneID b ) {
    return _bone[b].position;
}



void Head::updateHandMovement() {
	glm::vec3 transformedHandMovement;
	    
	transformedHandMovement 
	= _orientation.getRight() *  _movedHandOffset.x
	+ _orientation.getUp()	  * -_movedHandOffset.y * 0.5f
	+ _orientation.getFront() * -_movedHandOffset.y;
    
	_bone[ AVATAR_BONE_RIGHT_HAND ].position += transformedHandMovement;
    
	//if holding hands, add a pull to the hand...
	if ( _usingBodySprings ) {
		if ( _closestOtherAvatar != -1 ) {	
			if ( _mousePressed ) {
				
				/*
				glm::vec3 handShakePull( DEBUG_otherAvatarListPosition[ closestOtherAvatar ]);
				handShakePull -= _bone[ AVATAR_BONE_RIGHT_HAND ].position;
				
				handShakePull *= 1.0;
				 
				transformedHandMovement += handShakePull;
				*/
                
				_bone[ AVATAR_BONE_RIGHT_HAND ].position = _otherAvatarHandPosition[ _closestOtherAvatar ];				
			}
		}
	}
    
	//-------------------------------------------------------------------------------
	// determine the arm vector
	//-------------------------------------------------------------------------------
	glm::vec3 armVector = _bone[ AVATAR_BONE_RIGHT_HAND ].position;
	armVector -= _bone[ AVATAR_BONE_RIGHT_SHOULDER ].position;


	//-------------------------------------------------------------------------------
	// test to see if right hand is being dragged beyond maximum arm length
	//-------------------------------------------------------------------------------
	float distance = glm::length( armVector );
	
	//-------------------------------------------------------------------------------
	// if right hand is being dragged beyond maximum arm length...
	//-------------------------------------------------------------------------------	
	if ( distance > _maxArmLength ) {
		//-------------------------------------------------------------------------------
		// reset right hand to be constrained to maximum arm length
		//-------------------------------------------------------------------------------
		_bone[ AVATAR_BONE_RIGHT_HAND ].position = _bone[ AVATAR_BONE_RIGHT_SHOULDER ].position;
		glm::vec3 armNormal = armVector / distance;
		armVector = armNormal * _maxArmLength;
		distance = _maxArmLength;
		glm::vec3 constrainedPosition = _bone[ AVATAR_BONE_RIGHT_SHOULDER ].position;
		constrainedPosition += armVector;
		_bone[ AVATAR_BONE_RIGHT_HAND ].position = constrainedPosition;
	}
	
	/*
	//-------------------------------------------------------------------------------
	// keep arm from going through av body...
	//-------------------------------------------------------------------------------	
	glm::vec3 adjustedArmVector = _bone[ AVATAR_BONE_RIGHT_HAND ].position;
	adjustedArmVector -= _bone[ AVATAR_BONE_RIGHT_SHOULDER ].position;
	
	float rightComponent = glm::dot( adjustedArmVector, avatar.orientation.getRight() );

	if ( rightComponent < 0.0 )
	{
		_bone[ AVATAR_BONE_RIGHT_HAND ].position -= avatar.orientation.getRight() * rightComponent;
	}	
	*/
	
	//-----------------------------------------------------------------------------
	// set elbow position 
	//-----------------------------------------------------------------------------
	glm::vec3 newElbowPosition = _bone[ AVATAR_BONE_RIGHT_SHOULDER ].position;
	newElbowPosition += armVector * ONE_HALF;
//glm::vec3 perpendicular = glm::cross( frontDirection, armVector );
	glm::vec3 perpendicular = glm::cross( _orientation.getFront(), armVector );

	newElbowPosition += perpendicular * ( 1.0f - ( _maxArmLength / distance ) ) * ONE_HALF;
	_bone[ AVATAR_BONE_RIGHT_UPPER_ARM ].position = newElbowPosition;

	//-----------------------------------------------------------------------------
	// set wrist position 
	//-----------------------------------------------------------------------------
	glm::vec3 vv( _bone[ AVATAR_BONE_RIGHT_HAND ].position );
	vv -= _bone[ AVATAR_BONE_RIGHT_UPPER_ARM ].position;
	glm::vec3 newWristPosition = _bone[ AVATAR_BONE_RIGHT_UPPER_ARM ].position;
	newWristPosition += vv * 0.7f;
	_bone[ AVATAR_BONE_RIGHT_FOREARM ].position = newWristPosition;
    
    //  Set the vector we send for hand position to other people to be our right hand
    setHandPosition(_bone[ AVATAR_BONE_RIGHT_HAND ].position);

}



void Head::renderBody() {
	//-----------------------------------------
    //  Render bone positions as spheres
	//-----------------------------------------
	for (int b=0; b<NUM_AVATAR_BONES; b++) {
        //renderBoneAsBlock( (AvatarBoneID)b);
        
		if ( _usingBodySprings ) {
			glColor3fv( lightBlue );
			glPushMatrix();
				glTranslatef( _bone[b].springyPosition.x, _bone[b].springyPosition.y, _bone[b].springyPosition.z );
				glutSolidSphere( _bone[b].radius, 10.0f, 5.0f );
			glPopMatrix();
		}
		else {
			glColor3fv( skinColor );
			glPushMatrix();
				glTranslatef( _bone[b].position.x, _bone[b].position.y, _bone[b].position.z );
				glutSolidSphere( _bone[b].radius, 10.0f, 5.0f );
			glPopMatrix();
		}
	}

	//-----------------------------------------------------
    // Render lines connecting the bone positions
	//-----------------------------------------------------
	if ( _usingBodySprings ) {
		glColor3f( 0.4f, 0.5f, 0.6f );
		glLineWidth(3.0);

		for (int b=1; b<NUM_AVATAR_BONES; b++) {
            if ( _bone[b].parent != AVATAR_BONE_NULL ) {
                glBegin( GL_LINE_STRIP );
                glVertex3fv( &_bone[ _bone[ b ].parent ].springyPosition.x );
                glVertex3fv( &_bone[ b ].springyPosition.x );
                glEnd();
            }
		}
	}
	else {
		glColor3fv( skinColor );
		glLineWidth(3.0);

		for (int b=1; b<NUM_AVATAR_BONES; b++) {
            if ( _bone[b].parent != AVATAR_BONE_NULL ) {
                glBegin( GL_LINE_STRIP );
                glVertex3fv( &_bone[ _bone[ b ].parent ].position.x );
                glVertex3fv( &_bone[ b ].position.x);
                glEnd();
            }
		}
	}
	
	
	if (( _usingBodySprings ) && ( _mousePressed )) {
		glColor4f( 1.0, 1.0, 0.5, 0.5 );
		glPushMatrix();
			glTranslatef
			( 
				_bone[ AVATAR_BONE_RIGHT_HAND ].springyPosition.x, 
				_bone[ AVATAR_BONE_RIGHT_HAND ].springyPosition.y, 
				_bone[ AVATAR_BONE_RIGHT_HAND ].springyPosition.z 
			);
			glutSolidSphere( 0.03f, 10.0f, 5.0f );
		glPopMatrix();
	}
}


void Head::renderBoneAsBlock( AvatarBoneID b ) {

        glColor3fv( lightBlue );
        glPushMatrix();
            glTranslatef( _bone[b].springyPosition.x, _bone[b].springyPosition.y, _bone[b].springyPosition.z );
            glutSolidSphere( _bone[b].radius, 10.0f, 5.0f );
        glPopMatrix();
}




void Head::SetNewHeadTarget(float pitch, float yaw) {
    _head.pitchTarget = pitch;
    _head.yawTarget   = yaw;
}

// getting data from Android transmitte app
void Head::processTransmitterData(unsigned char* packetData, int numBytes) {
    //  Read a packet from a transmitter app, process the data
    float accX, accY, accZ,
    graX, graY, graZ,
    gyrX, gyrY, gyrZ,
    linX, linY, linZ,
    rot1, rot2, rot3, rot4;
    sscanf((char *)packetData, "tacc %f %f %f gra %f %f %f gyr %f %f %f lin %f %f %f rot %f %f %f %f",
           &accX, &accY, &accZ,
           &graX, &graY, &graZ,
           &gyrX, &gyrY, &gyrZ,
           &linX, &linY, &linZ,
           &rot1, &rot2, &rot3, &rot4);
    
    if (_transmitterPackets++ == 0) {
        gettimeofday(&_transmitterTimer, NULL);
    }
    const int TRANSMITTER_COUNT = 100;
    if (_transmitterPackets % TRANSMITTER_COUNT == 0) {
        // Every 100 packets, record the observed Hz of the transmitter data
        timeval now;
        gettimeofday(&now, NULL);
        double msecsElapsed = diffclock(&_transmitterTimer, &now);
        _transmitterHz = static_cast<float>( (double)TRANSMITTER_COUNT/(msecsElapsed/1000.0) );
        _transmitterTimer = now;
    }
    /*  NOTE:  PR:  Will add back in when ready to animate avatar hand

    //  Add rotational forces to the hand
    const float ANG_VEL_SENSITIVITY = 4.0;
    const float ANG_VEL_THRESHOLD = 0.0;
    float angVelScale = ANG_VEL_SENSITIVITY*(1.0f/getTransmitterHz());
    
    addAngularVelocity(fabs(gyrX*angVelScale)>ANG_VEL_THRESHOLD?gyrX*angVelScale:0,
                       fabs(gyrZ*angVelScale)>ANG_VEL_THRESHOLD?gyrZ*angVelScale:0,
                       fabs(-gyrY*angVelScale)>ANG_VEL_THRESHOLD?-gyrY*angVelScale:0);
    
    //  Add linear forces to the hand
    //const float LINEAR_VEL_SENSITIVITY = 50.0;
    const float LINEAR_VEL_SENSITIVITY = 5.0;
    float linVelScale = LINEAR_VEL_SENSITIVITY*(1.0f/getTransmitterHz());
    glm::vec3 linVel(linX*linVelScale, linZ*linVelScale, -linY*linVelScale);
    addVelocity(linVel);
    */
    
}

