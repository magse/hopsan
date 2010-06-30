//!
//! @file   HydraulicVolume.hpp
//! @author Björn Eriksson <bjorn.eriksson@liu.se>
//! @date   2009-12-19
//!
//! @brief Contains a Hydraulic Volume Component
//!
//$Id$

#ifndef HYDRAULICVOLUME_HPP_INCLUDED
#define HYDRAULICVOLUME_HPP_INCLUDED

#include "../../ComponentEssentials.h"

//!
//! @brief A hydraulic volume component
//! @ingroup HydraulicComponents
//!
class HydraulicVolume : public ComponentC
{

private:
    double mStartPressure;
    double mStartFlow;
    double mZc;
    double mAlpha;
    double mVolume;
    double mBulkmodulus;
    Port *mpP1, *mpP2;

    double debug,tid1,tid2;

public:
    static Component *Creator()
    {
        return new HydraulicVolume("Volume");
    }

    HydraulicVolume(const string name) : ComponentC(name)
    {
        //Set member attributes
        mTypeName = "HydraulicVolume";
        mStartPressure = 0.0;
        mStartFlow     = 0.0;
        mBulkmodulus   = 1.0e9;
        mVolume        = 1.0e-3;
        mAlpha         = 0.0;

        //Add ports to the component
        mpP1 = addPowerPort("P1", "NodeHydraulic");
        mpP2 = addPowerPort("P2", "NodeHydraulic");

        //Register changable parameters to the HOPSAN++ core
        registerParameter("V", "Volume", "[m^3]",            mVolume);
        registerParameter("Be", "Bulkmodulus", "[Pa]", mBulkmodulus);
        registerParameter("a", "Low pass coeficient to dampen standing delayline waves", "[-]",  mAlpha);

        tid1 = 0.0;
        tid2 = 0.01;
        debug = 0;
        registerParameter("debug", "debug", "[-]", debug);
        registerParameter("t1", "debug", "[-]", tid1);
        registerParameter("t2", "debug", "[-]", tid2);
    }


    void initialize()
    {
        mZc = mBulkmodulus/mVolume*mTimestep/(1-mAlpha); //Need to be updated at simulation start since it is volume and bulk that are set.

        //Write to nodes
        mpP1->writeNode(NodeHydraulic::MASSFLOW,     mStartFlow);
        mpP1->writeNode(NodeHydraulic::PRESSURE,     mStartPressure);
        mpP1->writeNode(NodeHydraulic::WAVEVARIABLE, mStartPressure+mZc*mStartFlow);
        mpP1->writeNode(NodeHydraulic::CHARIMP,      mZc);
        mpP2->writeNode(NodeHydraulic::MASSFLOW,     mStartFlow);
        mpP2->writeNode(NodeHydraulic::PRESSURE,     mStartPressure);
        mpP2->writeNode(NodeHydraulic::WAVEVARIABLE, mStartPressure+mZc*mStartFlow);
        mpP2->writeNode(NodeHydraulic::CHARIMP,      mZc);
    }


    void simulateOneTimestep()
    {
        //Get variable values from nodes
        double p1  = mpP1->readNode(NodeHydraulic::PRESSURE);
        double q1  = mpP1->readNode(NodeHydraulic::MASSFLOW);
        double c1  = mpP1->readNode(NodeHydraulic::WAVEVARIABLE);
        double p2  = mpP1->readNode(NodeHydraulic::PRESSURE);
        double q2  = mpP2->readNode(NodeHydraulic::MASSFLOW);
        double c2  = mpP2->readNode(NodeHydraulic::WAVEVARIABLE);

        //Volume equations

        //double c10 = p2 + mZc * q2;       //Why did we write these equations?
        //double c20 = p1 + mZc * q1;

        double c10 = c2 + 2.0*mZc * q2;     //These two equations are from old Hopsan
        double c20 = c1 + 2.0*mZc * q1;

        c1 = mAlpha*c1 + (1.0-mAlpha)*c10;
        c2 = mAlpha*c2 + (1.0-mAlpha)*c20;

        if ((mTime>tid1) && (mTime<tid2) && (debug > 0.5))
            std::cout << this->getName() << ": " << "mTime: " << mTime << "   p1: " << p1 << "   c1: " << c1 << "   q1: " << q1 << "   mZc: " << mZc << "   p2: " << p2 << "   c2: " << c2 << "   q2: " << q2 << std::endl;

        //Write new values to nodes
        mpP1->writeNode(NodeHydraulic::WAVEVARIABLE, c1);
        mpP2->writeNode(NodeHydraulic::WAVEVARIABLE, c2);
        mpP1->writeNode(NodeHydraulic::CHARIMP,      mZc);
        mpP2->writeNode(NodeHydraulic::CHARIMP,      mZc);
    }

    void finalize()
    {

    }
};

#endif // HYDRAULICVOLUME_HPP_INCLUDED
