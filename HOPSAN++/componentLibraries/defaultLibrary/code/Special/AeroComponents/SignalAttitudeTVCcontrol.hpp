#ifndef SIGNALATTITUDETVCCONTROL_HPP_INCLUDED
#define SIGNALATTITUDETVCCONTROL_HPP_INCLUDED

#include <iostream>
#include "ComponentEssentials.h"
#include "ComponentUtilities.h"
#include "math.h"

//!
//! @file SignalAttitudeTVCcontrol.hpp
//! @author Petter Krus <petter.krus@liu.se>
//! @date Thu 31 Jan 2013 11:57:18
//! @brief Attitude control unit for an aircraft
//! @ingroup SignalComponents
//!
//This component is generated by COMPGEN for HOPSAN-NG simulation 
//from 
/*{, C:, Users, petkr14.IEI, Documents, CompgenNG}/SignalControlAeroNG.nb*/

using namespace hopsan;

class SignalAttitudeTVCcontrol : public ComponentSignal
{
private:
     double mKelev;
     double mKrud;
     double mKQrud;
     double mKRrud;
     double mumin;
     double mumax;
     double mthetaref;
     double mpsiref;
     double mphi;
     double mtheta;
     double mpsi;
     double mQb;
     double mRb;
     Port *mpPthetaref;
     Port *mpPpsiref;
     Port *mpPphi;
     Port *mpPtheta;
     Port *mpPpsi;
     Port *mpPQb;
     Port *mpPRb;
     Port *mpPuelev;
     Port *mpPurud;
     int mNstep;
     //inputVariables
     double thetaref;
     double psiref;
     double phi;
     double theta;
     double psi;
     double Qb;
     double Rb;
     //outputVariables
     double uelev;
     double urud;

     //Expressions variables

     //Delay declarations
     //inputVariables pointers
     double *mpND_thetaref;
     double *mpND_psiref;
     double *mpND_phi;
     double *mpND_theta;
     double *mpND_psi;
     double *mpND_Qb;
     double *mpND_Rb;
     //outputVariables pointers
     double *mpND_uelev;
     double *mpND_urud;
     EquationSystemSolver *mpSolver;

public:
     static Component *Creator()
     {
        return new SignalAttitudeTVCcontrol();
     }

     void configure()
     {
        const double Kelev = 4.;
        const double Krud = 4.;
        const double KQrud = 1.;
        const double KRrud = 1.;
        const double umin = -0.9;
        const double umax = 0.9;
        const double thetaref = 0.;
        const double psiref = 0.;
        const double phi = 0.;
        const double theta = 0.;
        const double psi = 0.;
        const double Qb = 0.;
        const double Rb = 0.;

        mNstep=9;
        mKelev = Kelev;
        mKrud = Krud;
        mKQrud = KQrud;
        mKRrud = KRrud;
        mumin = umin;
        mumax = umax;
        mthetaref = thetaref;
        mpsiref = psiref;
        mphi = phi;
        mtheta = theta;
        mpsi = psi;
        mQb = Qb;
        mRb = Rb;

        //Add ports to the component

        //Add inputVariables ports to the component
        mpPthetaref=addReadPort("Pthetaref","NodeSignal", Port::NOTREQUIRED);
        mpPpsiref=addReadPort("Ppsiref","NodeSignal", Port::NOTREQUIRED);
        mpPphi=addReadPort("Pphi","NodeSignal", Port::NOTREQUIRED);
        mpPtheta=addReadPort("Ptheta","NodeSignal", Port::NOTREQUIRED);
        mpPpsi=addReadPort("Ppsi","NodeSignal", Port::NOTREQUIRED);
        mpPQb=addReadPort("PQb","NodeSignal", Port::NOTREQUIRED);
        mpPRb=addReadPort("PRb","NodeSignal", Port::NOTREQUIRED);

        //Add outputVariables ports to the component
        mpPuelev=addWritePort("Puelev","NodeSignal", Port::NOTREQUIRED);
        mpPurud=addWritePort("Purud","NodeSignal", Port::NOTREQUIRED);

        //Register changable parameters to the HOPSAN++ core
        registerParameter("Kelev", "Gain tip, default", "rad", mKelev);
        registerParameter("Krud", "Gain yaw, default", "rad", mKrud);
        registerParameter("KQrud", "Gain tip rate, default", "", mKQrud);
        registerParameter("KRrud", "Gain yaw rate, default", "", mKRrud);
        registerParameter("umin", "Minium output signal roll", "rad", mumin);
        registerParameter("umax", "Maximum output signal roll", "rad", \
mumax);
        registerParameter("thetaref", "Reference signal tip", "rad", \
mthetaref);
        registerParameter("psiref", "Reference signal yaw", "rad", mpsiref);
        registerParameter("phi", "roll angle", "rad", mphi);
        registerParameter("theta", "tipp angle", "rad", mtheta);
        registerParameter("psi", "yaw angle", "rad", mpsi);
        registerParameter("Qb", "tip angle rate", "rad/s", mQb);
        registerParameter("Rb", "yaw angle rate", "rad/s", mRb);
     }

    void initialize()
     {
        //Read port variable pointers from nodes
        //Read inputVariables pointers from nodes
        mpND_thetaref=getSafeNodeDataPtr(mpPthetaref, \
NodeSignal::VALUE,mthetaref);
        mpND_psiref=getSafeNodeDataPtr(mpPpsiref, NodeSignal::VALUE,mpsiref);
        mpND_phi=getSafeNodeDataPtr(mpPphi, NodeSignal::VALUE,mphi);
        mpND_theta=getSafeNodeDataPtr(mpPtheta, NodeSignal::VALUE,mtheta);
        mpND_psi=getSafeNodeDataPtr(mpPpsi, NodeSignal::VALUE,mpsi);
        mpND_Qb=getSafeNodeDataPtr(mpPQb, NodeSignal::VALUE,mQb);
        mpND_Rb=getSafeNodeDataPtr(mpPRb, NodeSignal::VALUE,mRb);
        //Read outputVariable pointers from nodes
        mpND_uelev=getSafeNodeDataPtr(mpPuelev, NodeSignal::VALUE);
        mpND_urud=getSafeNodeDataPtr(mpPurud, NodeSignal::VALUE);

        //Read variables from nodes

        //Read inputVariables from nodes
        thetaref = (*mpND_thetaref);
        psiref = (*mpND_psiref);
        phi = (*mpND_phi);
        theta = (*mpND_theta);
        psi = (*mpND_psi);
        Qb = (*mpND_Qb);
        Rb = (*mpND_Rb);

        //Read outputVariables from nodes
        uelev = mpPuelev->getStartValue(NodeSignal::VALUE);
        urud = mpPurud->getStartValue(NodeSignal::VALUE);



        //Initialize delays

     }
    void simulateOneTimestep()
     {
        //Read variables from nodes

        //Read inputVariables from nodes
        thetaref = (*mpND_thetaref);
        psiref = (*mpND_psiref);
        phi = (*mpND_phi);
        theta = (*mpND_theta);
        psi = (*mpND_psi);
        Qb = (*mpND_Qb);
        Rb = (*mpND_Rb);

        //LocalExpressions

          //Expressions
          uelev = limit(-(mKelev*(-(mKQrud*Qb) + \
diffAngle(thetaref,theta))),mumin,mumax);
          urud = limit(-(mKrud*(-(mKRrud*Rb) + \
diffAngle(psiref,psi))),mumin,mumax);

        //Calculate the delayed parts


        //Write new values to nodes
        //outputVariables
        (*mpND_uelev)=uelev;
        (*mpND_urud)=urud;

        //Update the delayed variabels

     }
};
#endif // SIGNALATTITUDETVCCONTROL_HPP_INCLUDED
