#ifndef OSCHANDLER_H_INCLUDED
#define OSCHANDLER_H_INCLUDED

#include "../JuceLibraryCode/JuceHeader.h"
#include "Utils.h"
#include <map>
#include <vector>
#include <math.h>

class OSCHandler :
    public Component,
    private juce::OSCReceiver,
    public OSCReceiver::Listener<OSCReceiver::MessageLoopCallback>,
    public ChangeBroadcaster
{

//==========================================================================
// ATTRIBUTES
    
private:
    
int port = 3860;

std::map<int,EL_ImageSource> sourceImageMap;
std::map<String,EL_Source> sourceMap;
std::map<String,EL_Listener> listenerMap;
std::vector<float> valuesR60;

//==========================================================================
// METHODS

public:

OSCHandler()
{
    // specify here on which UDP port number to receive incoming OSC messages
    if( !connect(port) )
    {
        showConnectionErrorMessage ("Error: (OSC) could not connect to localhost@" + String(port) + ".");
    }
    
    addListener (this);
    valuesR60.resize(NUM_OCTAVE_BANDS, 0.f);
}

~OSCHandler() {}

// TODO: SETUP FOR MULTI-USER / MULTI-SOURCE
std::vector<int> getSourceImageIDs()
{
    std::vector<int> IDs;
    IDs.resize(sourceImageMap.size());
    int i = 0;
    for(auto const &ent1 : sourceImageMap) {
        
        IDs[i] = ent1.first;
        i ++;
    }
    return IDs;
}

// TODO: SETUP FOR MULTI-USER / MULTI-SOURCE
std::vector<float> getSourceImageDelays()
{
    std::vector<float> delays;
    delays.resize(sourceImageMap.size());
    int i = 0;
    for(auto const &ent1 : sourceImageMap) {
        
        delays[i] = ent1.second.totalPathDistance / SOUND_SPEED;
        i ++;
    }
    return delays;
}

// TODO: SETUP FOR MULTI-USER / MULTI-SOURCE
std::vector<float> getSourceImagePathsLength()
{
    std::vector<float> pathLength;
    pathLength.resize(sourceImageMap.size());
    int i = 0;
    for(auto const &ent1 : sourceImageMap) {
        
        pathLength[i] = ent1.second.totalPathDistance;
        i ++;
    }
    return pathLength;
}

// TODO: SETUP FOR MULTI-USER / MULTI-SOURCE
std::vector<Eigen::Vector3f> getSourceImageDOAs()
{
    Eigen::Vector3f listenerPos = listenerMap.begin()->second.position;
    Eigen::Matrix3f listenerRotationMatrix = listenerMap.begin()->second.rotationMatrix;
    
    std::vector<Eigen::Vector3f> doas;
    doas.resize(sourceImageMap.size());
    
    int i = 0;
    for(auto const &ent1 : sourceImageMap) {
        Eigen::Vector3f posSph = cartesianToSpherical( listenerRotationMatrix * ( ent1.second.positionRelectionLast - listenerPos ) );
        doas[i] = posSph;
        i ++;
    }
    return doas;
}

// TODO: SETUP FOR MULTI-USER / MULTI-SOURCE
Array<float> getSourceImageAbsorption(int sourceID)
{
    return sourceImageMap.find(sourceID)->second.absorption;
}

std::vector<float> getRT60Values()
{
    return valuesR60;
}
    
int getDirectPathId()
{
    for(auto const &ent1 : sourceImageMap) {
        if( ent1.second.reflectionOrder == 0) {
            return ent1.first;
        }
    }
    return -1;
}

String getMapContent()
{
    String output;
    int nDecimals = 2;
    
    for(auto const &ent1 : listenerMap) {
        // ent1.first is the first key
        output += String("Listener: \t") + String(ent1.first) + String(", pos: \t[ ") +
        String(round2(ent1.second.position(0), nDecimals)) + String(", ") +
        String(round2(ent1.second.position(1), nDecimals)) + String(", ") +
        String(round2(ent1.second.position(2), nDecimals)) + String(" ]\n");
    }
    output += String("\n");
    
    for(auto const &ent1 : sourceMap) {
        // ent1.first is the first key
        output += String("Source:  \t") + String(ent1.first) + String(", pos: \t [ ") +
        String(round2(ent1.second.position(0), nDecimals)) + String(", ") +
        String(round2(ent1.second.position(1), nDecimals)) + String(", ") +
        String(round2(ent1.second.position(2), nDecimals)) + String(" ]\n");
    }
    output += String("\n");
    
    output += String("RT60: \t[ ");
    for( int i = 0; i < valuesR60.size(); i++ ){
        output += String(round2(valuesR60[i], nDecimals));
        if( i < valuesR60.size() - 1 ) output += String(", ");
        else output += String(" ] (sec) \n");
    }
    output += String("\n");
    
    Eigen::Vector3f listenerPos = listenerMap.begin()->second.position;
    std::vector<Eigen::Vector3f> posSph = getSourceImageDOAs();
    int i = 0;
    for(auto const &ent1 : sourceImageMap) {
        // ent1.first is the first key
        // Eigen::Vector3f posSph = cartesianToSpherical(ent1.second.positionRelectionLast - listenerPos);
        
        output += String("Source Image: ") + String(ent1.first) + String(", \t posLast:  [ ") +
        String(round2(ent1.second.positionRelectionLast(0), nDecimals)) + String(", ") +
        String(round2(ent1.second.positionRelectionLast(1), nDecimals)) + String(", ") +
        String(round2(ent1.second.positionRelectionLast(2), nDecimals)) + String(" ]");
        
        output += String(", \t DOA: [ ") +
        String(round2(rad2deg(posSph[i](0)), nDecimals)) + String(", ") +
        String(round2(rad2deg(posSph[i](1)), nDecimals)) + String(" ]");
        i++;
        
        output += String(", \t path length: ") + String(round(ent1.second.totalPathDistance*100)/100) + String("m\n");
    }
    
    return output;
}

    
private:

void oscMessageReceived (const OSCMessage& msg) override
{
    OSCAddressPattern pIn("/in");
    OSCAddressPattern pUpd("/upd");
    OSCAddressPattern pR60("/r60");
    
    OSCAddress msgAdress(msg.getAddressPattern().toString());
    
    // DBG(String("osc msg: ") + msg.getAddressPattern().toString() + String(" size: ") + String(msg.size()) + String(" id: ") + String(msg[0].getInt32()));
    
    if ( (pIn.matches(msgAdress) || pUpd.matches(msgAdress) ) && msg.size() == 19){
        // format: [ /in pathID order r1x r1y r1z rNx rNy rNz dist abs1 .. abs9 ]
        
        EL_ImageSource source;
        source.ID = msg[0].getInt32();
        source.reflectionOrder = msg[1].getInt32();
        
        //        for (int i = 1; i < 3; i++)
        //            source.positionRelectionFirst[i] = msg[2+i].getFloat32();
        source.positionRelectionFirst(0) = msg[2].getFloat32();
        source.positionRelectionFirst(1) = msg[3].getFloat32();
        source.positionRelectionFirst(2) = msg[4].getFloat32();
        
        //        for (int i = 1; i < 3; i++)
        //            source.positionRelectionLast[i] = msg[5+i].getFloat32();
        source.positionRelectionLast(0) = msg[5].getFloat32();
        source.positionRelectionLast(1) = msg[6].getFloat32();
        source.positionRelectionLast(2) = msg[7].getFloat32();
        
        source.totalPathDistance = msg[8].getFloat32();
        
        for (int i = 0; i < 10; i++)
            source.absorption.insert(i, msg[9+i].getFloat32());
        
        // insert or update
        sourceImageMap[source.ID] = source;
    }
    
    else if( pR60.matches(msgAdress) )
    {
        for( int i = 0; i < msg.size(); i++ ){ valuesR60[i] = msg[i].getFloat32(); }
//        DBG(String("r60: ") + String(valuesR60[0]) + String(", ") + String(valuesR60[1]) + String(", ") + String(valuesR60[2]) + String(", ") + String(valuesR60[3]) + String(", ") + String(valuesR60[4]) + String(", ") + String(valuesR60[5]) + String(", ") + String(valuesR60[6]) + String(", ") + String(valuesR60[7]) + String(", ") + String(valuesR60[8]) + String(", ") + String(valuesR60[9]) + String(" "));
    }
    
    //    else {
    //        DBG(String("unhandled osc msg: ") + msg.getAddressPattern().toString() + String(" size: ") + String(msg.size()));
    //
    //        for (int i = 1; i < msg.size(); i++) {
    //            if (msg[i].isInt32()) DBG(String(i) + String(" Int32") );
    //            if (msg[i].isFloat32()) DBG(String(i) + String(" Float32") );
    //            if (msg[i].isString()) DBG(String(i) + String(" String") );
    //            if (msg[i].isBlob()) DBG(String(i) + String(" Blob") );
    //        }
    //    }
    
    sendChangeMessage ();
    
}

void oscBundleReceived (const OSCBundle & bundle) override
{
    OSCAddressPattern pSource("/source");
    OSCAddressPattern pListener("/listener");
    OSCAddressPattern pOut("/out");
    
    //    DBG(bundle.size());
    
    for (int i = 0; i < bundle.size(); i++)
    {
        OSCMessage msg = bundle[i].getMessage();
        OSCAddress msgAdress(msg.getAddressPattern().toString());
        // DBG("osc msg (bundle " + String(i) + String("):") + msgAdress.toString() + String(" size: ") + String(msg.size()));
        
        //        for (int i = 0; i < msg.size(); i++) {
        //            if (msg[i].isInt32()) DBG(String(i) + String(" Int32") );
        //            if (msg[i].isFloat32()) DBG(String(i) + String(" Float32") );
        //            if (msg[i].isString()) DBG(String(i) + String(" String") );
        //            if (msg[i].isBlob()) DBG(String(i) + String(" Blob") );
        //        }
        
        if ( pSource.matches(msgAdress) )
        {
            EL_Source source;
            source.name = msg[0].getString();
            source.position(0) = msg[1].getFloat32();
            source.position(1) = msg[2].getFloat32();
            source.position(2) = msg[3].getFloat32();
            
            // insert or update
            sourceMap[source.name] = source;
        }
        else if ( pListener.matches(msgAdress) )
        {
            EL_Listener listener;
            listener.name = msg[0].getString();
            listener.position(0) = msg[1].getFloat32();
            listener.position(1) = msg[2].getFloat32();
            listener.position(2) = msg[3].getFloat32();
            
            for (int j = 0; j < 3; j++)
            {
                for (int k = 0; k < 3; k++)
                {
                    listener.rotationMatrix(j,k) = msg[4+ (3*j + k)].getFloat32();
                }
            }
            
            // insert or update
            listenerMap[listener.name] = listener;
        }
        else if ( pOut.matches(msgAdress) )
        {
            sourceImageMap.erase(msg[0].getInt32());
        }
    }
    
    sendChangeMessage ();
}

// popup window if OSC connection failed 
void showConnectionErrorMessage (const String& messageText)
{
    AlertWindow::showMessageBoxAsync (
                                      AlertWindow::WarningIcon,
                                      "Connection error",
                                      messageText,
                                      "OK");
}

    
JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OSCHandler)
    
};

#endif // OSCHANDLER_H_INCLUDED
