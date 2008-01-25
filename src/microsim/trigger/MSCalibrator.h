/****************************************************************************/
/// @file    MSCalibrator.h
/// @author  Tino Morenz
/// @date    Wed, 24.10.2007
/// @version $Id: $
///
// A vehicle emitting device
/****************************************************************************/
// SUMO, Simulation of Urban MObility; see http://sumo.sourceforge.net/
// copyright : (C) 2001-2007
//  by DLR (http://www.dlr.de/) and ZAIK (http://www.zaik.uni-koeln.de/AFS)
/****************************************************************************/
//
//   This program is free software; you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation; either version 2 of the License, or
//   (at your option) any later version.
//
/****************************************************************************/
#ifndef MSCalibrator_h
#define MSCalibrator_h


// ===========================================================================
// included modules
// ===========================================================================
#ifdef _MSC_VER
#include <windows_config.h>
#else
#include <config.h>
#endif

#include <string>
#include <vector>
#include <utils/common/Command.h>
#include "MSTriggeredXMLReader.h"
#include "MSTrigger.h"
#include <utils/common/RandomDistributor.h>

#include <netload/NLDetectorBuilder.h>

//#define protected public
//#define private public


// ===========================================================================
// class declarations
// ===========================================================================
class MSNet;
class MSLane;
class NLDetectorBuilder;
class MSInductLoop;

// ===========================================================================
// class definitions
// ===========================================================================
/**
 * @class MSCalibrator
 * @brief A device to calibrate the number of vehicles on a lane
 *
 */
class MSCalibrator : public MSTrigger
{
public:
    MSCalibrator(const std::string &id, MSNet &net, MSLane* destLane,
                 SUMOReal pos, const std::string &aXMLFilename) throw();

    virtual ~MSCalibrator() throw();


public:
    /**
     * @class MSCalibratorChild
     * @brief Holder of information about vehicle routes and types
     *
     * Classes which realise different schemes to emit vehicles (file-based,
     *  based on user entries on the gui, etc.) should subclass this class
     */
    class MSCalibratorChild
    {
    public:
        /// Constructor
        MSCalibratorChild(MSCalibrator &parent, MSVehicleControl &vc)
                : myParent(parent), myVehicleControl(vc), myTimeOffset(0) { }

        /// Destructor
        virtual ~MSCalibratorChild() { }

        /// Returns a random route
        MSRoute *getRndRoute() const {
            return myRouteDist.get();
        }

        /// Returns a list of all available routes
        const std::vector<MSRoute*> &getAllRoutes() const {
            return myRouteDist.getVals();
        }

        /// Returns a random type
        MSVehicleType *getRndVType() const {
            return myVTypeDist.get();
        }

        /// Returns the information whether any route is stored
        bool hasRoutes() const {
            return myRouteDist.getOverallProb()!=0;
        }

        /// Returns the information whether any vehicle type is stored
        bool hasVTypes() const {
            return myVTypeDist.getOverallProb()!=0;
        }

        /// Returns the time offset till the next vehicle emission for a given flow
        SUMOReal computeOffset(SUMOReal flow) const {
            SUMOReal freq = (SUMOReal)(1. / (flow / 3600.));
            SUMOReal ret = freq;
            myTimeOffset += (freq - (SUMOTime) ret);
            if (myTimeOffset>1) {
                myTimeOffset -= 1;
                ret += 1;
            }
            if (ret==0) { // !!! check what to do in this case (more than one vehicle/s)
                ret = 1;
            }
            return ret;
        }

        /// Returns the routes probability
        RandomDistributor<MSRoute*> &getRouteDist() {
            return myRouteDist;
        }

    protected:
        /// The calibrator this instance is child of
        MSCalibrator &myParent;

        /// The vehicle control to use for vehicle building
        MSVehicleControl &myVehicleControl;

        /// The used route probability distribution
        RandomDistributor<MSRoute*> myRouteDist;

        /// The used vehicle type probability distribution
        RandomDistributor<MSVehicleType*> myVTypeDist;

        /// The time offset till next emission
        mutable SUMOReal myTimeOffset;

    };

public:
    /** Called by a MSCalibratorChild (given as parameter) this method checks whether
     * this child is the currently active one and whether a vehicle from this source
     * can be emitted.
     * Returns true if the child can continue with the next vehicle, false otherwise */
    bool childCheckEmit(MSCalibratorChild *child);

    /// Returns the index of the currently active child
    size_t getActiveChildIndex() const;

    /// Sets the given child as the currently used one
    void setActiveChild(MSCalibratorChild *child);


protected:
    /**
     * @class MSCalibrator_FileTriggeredChild
     * @brief An CalibratorChild which uses information from a parsed file
     */
    class MSCalibrator_FileTriggeredChild
                : public MSTriggeredXMLReader, public MSCalibratorChild, public Command
    {
    public:
        /// Constructor
        MSCalibrator_FileTriggeredChild(MSNet &net,
                                        const std::string &aXMLFilename, MSCalibrator &parent, MSVehicleControl &vc);

        /// Destructor
        ~MSCalibrator_FileTriggeredChild() throw();

        /** the implementation of the MSTriggeredReader-processNextEntryReaderTriggered method */
        bool processNextEntryReaderTriggered();

        /// Builds a vehicle and tries to emit it
        SUMOTime execute(SUMOTime currentTime);

        /// Returns the loaded flow
        SUMOReal getLoadedFlow() const;

    protected:
        /// @name inherited from GenericSAXHandler
        //@{
        /** the implementation of the SAX-handler interface for reading
            element begins */
        virtual void myStartElement(SumoXMLTag element,
                                    const Attributes &attrs) throw(ProcessError);
        //@}

        /// Informs the child that the end of the processed file has been reached
        void inputEndReached();

        /// Returns the information whether a next valid entry has been reached
        // !!! seee
        bool nextRead();
        //TM
    public:
        /// Builds a vehicle and schedules it for further processing
        void buildAndScheduleFlowVehicle() {
            buildAndScheduleFlowVehicle(-1);
        }
        void buildAndScheduleFlowVehicle(SUMOReal speed);

    protected:
        /// Information whether a further vehicle to emit exists
        bool myHaveNext;

        /// The last loaded flow
        SUMOReal myFlow;

        /// Information whether the flow is used
        bool myHaveInitialisedFlow;

        /// A running vehicle id
        int myRunningID;

        /// Information about the simulation time the processing starts at; previous vehicles are discarded
        SUMOTime myBeginTime;

    };

public:
    /// Schedules a vehicle to emit
    void schedule(MSCalibratorChild *child, MSVehicle *v, SUMOReal speed);

    //TM
    NLDetectorBuilder myDb;
    MSInductLoop * myIL;
    SUMOReal myToCalibrate;
    SUMOTime myInterval;
    SUMOReal myNumVehicles;
    int myDebugLevel;
    std::string myDebugFilesBaseName;

    static void updateCalibrator(std::string name, int time, SUMOReal count);

protected:
    /// The network the calibrator is located in
    MSNet &myNet;

    /** the lane the calibrator is placed on */
    MSLane *myDestLane;

    /// The position of the calibrator at the lane
    SUMOReal myPos;


    /// The file-based child
    MSCalibratorChild *myFileBasedCalibrator;

    /// A map children->vehicles to emit
    std::map<MSCalibratorChild*, std::pair<MSVehicle*, SUMOReal> > myToEmit;

    /// The currently active child
    MSCalibratorChild *myActiveChild;


    /// TM
    SUMOTime execute(SUMOTime currentTime);
    static std::map<std::string, MSCalibrator*> calibratorMap;


};


#endif

/****************************************************************************/

