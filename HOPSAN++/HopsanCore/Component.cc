//!
//! @file   Component.cc
//! @author FluMeS
//! @date   2009-12-20
//!
//! @brief Contains Component base classes as well as Component Parameter class
//!
//$Id$

//! @defgroup Components Components

#include <iostream>
#include <sstream>
#include <cassert>
#include <math.h>
#include "Component.h"
#include "Port.h"

//Constructor
CompParameter::CompParameter(const string name, const string description, const string unit, double &rValue)
{
    mName = name;
    mDescription = description;
    mUnit = unit;
    mpValue = &rValue;
};


string CompParameter::getName()
{
    return mName;
}

string CompParameter::getDesc()
{
    return mDescription;
}


string CompParameter::getUnit()
{
    return mUnit;
}


double CompParameter::getValue()
{
    return *mpValue;
}


void CompParameter::setValue(const double value)
{
    *mpValue = value;
}


//Constructor
Component::Component(string name, double timestep)
{
    mName = name;
    mTimestep = timestep;

    mIsComponentC = false;
    mIsComponentQ = false;
    mIsComponentSystem = false;
    mIsComponentSignal = false;
    mTypeCQS = "";

    mpSystemParent = 0;

    //registerParameter("Ts", "Sample time", "[s]",   mTimestep);
}

//! Virtual Function, base version which gives you an error if you try to use it.
void Component::initialize(const double startT, const double stopT)
{
    cout << "Error! This function should only be used by system components, it should be overloded. For a component use initialize() instead" << endl;
    assert(false);
}

void Component::simulate(const double startT, const double stopT)
{
//TODO: adjust self.timestep or simulation depending on Ts from system above (self.timestep should be multipla of Ts)
    //double dT = stopT-startT;
    double stopTsafe = stopT - mTimestep/2.0;
    mTime = startT;
    while (mTime < stopTsafe)
    {
        simulateOneTimestep();
        mTime += mTimestep;
    }
    //cout << "simulate in: " << this->getName() << endl;
}

void Component::initialize()
{
    cout << "Warning! You should implement your own method" << endl;
    assert(false);
}

void Component::simulateOneTimestep()
{
    cout << "Warning! You should implement your own method" << endl;
    assert(false);
}

void Component::setName(string name)
{
    //! @todo fix the trailing _ removal
    //First strip any lonely trailing _ from the name (and give a warning)
//    string::iterator lastchar = --(name.end());
//    while (*lastchar == "_")
//    {
//        cout << "Warning underscore is not alowed in the end of a name (to avoid ugly collsion with name suffix)" << endl;
//        name.erase(lastchar);
//        lastchar = --(name.end());
//    }

    //If name same as before do nothing
    if (name != mName)
    {
        //Do we have a system parent
        if (mpSystemParent != 0)
        {
            //If the old name has not already been changed, let it decide our new name
            //Need this to prevent risk of loop between rename and this function (rename cals this one again)
            if (mpSystemParent->haveSubComponent(mName))
            {
                //Rename
                mpSystemParent->renameSubComponent(mName, name);
            }
            else
            {
                mName = name;
            }
        }
        else
        {
            //Ok no systemparent is set yet so lets set our own name
            mName = name;
        }
    }
}

const string &Component::getName()
{
    return mName;
}

const string &Component::getTypeCQS()
{
    return mTypeCQS;
}

const string &Component::getTypeName()
{
    return mTypeName;
}

void Component::registerParameter(const string name, const string description, const string unit, double &rValue)
{
    ///TODO: handle trying to add multiple comppar with same name or pos
    CompParameter new_comppar(name, description, unit, rValue);
    mParameters.push_back(new_comppar); //Copy parameters into storage
}

void Component::listParametersConsole()
{
    cout <<"-----------------------------------------------" << endl << getName() << ":" << endl;
    for (size_t i=0; i<mParameters.size(); ++i)
    {
        cout << "Parameter " << i << ": " << mParameters[i].getName() << " = " << mParameters[i].getValue() << " " << mParameters[i].getUnit() << " " << mParameters[i].getDesc() << endl;
    }
    cout <<"-----------------------------------------------" << endl;
}

double Component::getParameter(const string name)
{
    for (size_t i=0; i<mParameters.size(); ++i)
    {
        if (mParameters[i].getName() == name)
        {
            return mParameters[i].getValue();
        }
    }
    cout << "No such parameter" << endl;
    assert(false);
    return 0.0;
}

vector<CompParameter> Component::getParameterVector()
{
    return mParameters;
}

map<string, double> Component::getParameterMap()
{
    map<string, double> parameterMap;
    for (size_t i=0; i<mParameters.size(); ++i)
    {
        parameterMap.insert(pair<string, double>(mParameters[i].getName(), mParameters[i].getValue()));
    }
    return parameterMap;
}

void Component::setParameter(const string name, const double value)
{
    bool notset = 1;
    for (size_t i=0; i<mParameters.size(); ++i)
    {
        if (name == mParameters[i].getName())
        {
            mParameters[i].setValue(value);
            notset = 0;
        }
    }
    if (notset)
    {
        cout << "No such parameter" << endl;
        assert(false);
    }
}

vector<Port*> Component::getPortPtrVector()
{
   return mPortPtrs;
}


//void Component::setTimestep(const double timestep)
//{
//    mTimestep = timestep;
//}

//double Component::getTimestep()
//{
//    return mTimestep;
//}

void Component::setDesiredTimestep(const double timestep)
{
    cout << "Warning this function setDesiredTimestep is only available on subsystem components" << endl;
    assert(false);
}

bool Component::isComponentC()
{
    return mIsComponentC;
}

bool Component::isComponentQ()
{
    return mIsComponentQ;
}

bool Component::isComponentSystem()
{
    return mIsComponentSystem;
}

bool Component::isComponentSignal()
{
    return mIsComponentSignal;
}


Port* Component::addPort(const string portname, const string porttype, const NodeTypeT nodetype, const int id)
{
    ///TODO: handle trying to add multiple ports with same name or pos
    Port* new_port = CreatePort(porttype);
    new_port->mPortName = portname;
    new_port->mNodeType = nodetype;
    new_port->mpComponent = this;    //Set port owner

    if (id >= 0)
    {
        //Instead of allways push_back, make it possible to add ports out of order
        if ((size_t)id+1 > mPortPtrs.size())
        {
            mPortPtrs.resize(id+1);
        }
        mPortPtrs[id] = new_port;
    }
    else
    {
        //If no id specified push back
        mPortPtrs.push_back(new_port);     //Copy port into storage
    }

    return new_port;
}


Port* Component::addPowerPort(const string portname, const string nodetype, const int id)
{
    return addPort(portname, "PowerPort", nodetype, id);
}

Port* Component::addReadPort(const string portname, const string nodetype, const int id)
{
    return addPort(portname, "ReadPort", nodetype, id);
}

Port* Component::addWritePort(const string portname, const string nodetype, const int id)
{
    return addPort(portname, "WritePort", nodetype, id);
}

//void Component::addMultiPort(const string portname, const string nodetype, const size_t nports, const size_t startctr)
//{
//    for (size_t idx=startctr; idx < nports+startctr; ++idx)
//    {
//        sstream ss;
//        ss << portname << idx;
//        addPort(ss.str(), nodetype);
//    }
//}

void Component::setSystemParent(ComponentSystem &rComponentSystem)
{
    mpSystemParent = &rComponentSystem;
}

Port &Component::getPortById(const size_t port_idx)
{
    ///TODO: error handle if request outside of vector
    return *mPortPtrs[port_idx];
}

Port &Component::getPort(const string portname)
{
    vector<Port*>::iterator it;
    for (it=mPortPtrs.begin(); it!=mPortPtrs.end(); ++it)
    {
        if ((*it)->mPortName == portname)
        {
            return *(*it);
        }
    }
    ///TODO: cast not found exception
    cout << "specified port: " << portname << " not found" << endl;
    assert(false);
}

bool Component::getPort(const string portname, Port* &rpPort)
{
    vector<Port*>::iterator it;
    for (it=mPortPtrs.begin(); it!=mPortPtrs.end(); ++it)
    {
        if ((*it)->mPortName == portname)
        {
            rpPort = (*it);
            return true;
        }
    }
    return false;
}

void Component::setTimestep(const double timestep)
{
    mTimestep = timestep;
}


string ComponentSystem::SubComponentStorage::modifyName(string name)
{
    cout << "Modified name: " << name << "  was changed to:  ";
    size_t ctr = 1; //The suffix number
    while(mSubComponentMap.count(name) != 0)
    {
        //strip suffix
        size_t foundpos = name.rfind("_");
        if (foundpos != string::npos)
        {
            if (foundpos+1 < name.size())
            {
                unsigned char nr = name.at(foundpos+1);
                //cout << "nr after _: " << nr << endl;
                //Check the ascii code for the charachter
                if ((nr >= 48) && (nr <= 57))
                {
                    //Is number lets assume that the _ found is the beginning of a suffix
                    name.erase(foundpos, string::npos);
                }
            }
        }
        //cout << "ctr: " << ctr << " stripped tempname: " << name << endl;

        //add new suffix
        stringstream suffix;
        suffix << ctr;
        name.append("_");
        name.append(suffix.str());
        ++ctr;
        //cout << "ctr: " << ctr << " appended tempname: " << name << endl;
    }
    cout << name << endl;
    return name;
}


//! The subcomponent storage, Makes it easier to add (with auto unique name), erase and get components
//! @todo quite ugly code for now
void ComponentSystem::SubComponentStorage::add(Component* pComponent)
{
    //First check if the name already exists, in that case change the suffix
    string tempname = pComponent->getName();
    //cout << "initial tempname: " << tempname << endl;

    tempname = modifyName(tempname);

    //Add to the cqs component vectors, remember te idx for the info
    int idx;
    if (pComponent->isComponentC())
    {
        mComponentCptrs.push_back(pComponent);
        idx = mComponentCptrs.size()-1;
    }
    else if (pComponent->isComponentQ())
    {
        mComponentQptrs.push_back(pComponent);
        idx = mComponentQptrs.size()-1;
    }
    else if (pComponent->isComponentSignal())
    {
        mComponentSignalptrs.push_back(pComponent);
        idx = mComponentSignalptrs.size()-1;
    }
    else
    {
        ///TODO: use exception instead
        cout << "Trying to add module of other type than c, q or signal" << endl;
        assert(false);
    }

    pComponent->setName(tempname);
    SubComponentInfo info;
    info.cqs_type = pComponent->getTypeCQS();
    info.idx = idx;

    mSubComponentMap.insert(pair<string, SubComponentInfo>(tempname, info));
}

Component* ComponentSystem::SubComponentStorage::get(string name)
{
    map<string, SubComponentInfo>::iterator it;
    it = mSubComponentMap.find(name);
    if (it != mSubComponentMap.end())
    {
        if (it->second.cqs_type == "C")
        {
            return mComponentCptrs[it->second.idx];
        }
        else if (it->second.cqs_type == "Q")
        {
            return mComponentQptrs[it->second.idx];
        }
        else if (it->second.cqs_type == "S")
        {
            return mComponentSignalptrs[it->second.idx];
        }
        else
        {
            cout << "This should not happen neither C Q or S type is set in the info" << endl;
            assert(false);
        }
    }
    else
    {
        //! @todo exception or similar instead
        cout << "The component you requested: " << name << " does not exist" << endl;
        assert(false);
    }
}

void ComponentSystem::SubComponentStorage::erase(string name)
{
    map<string, SubComponentInfo>::iterator it;
    it = mSubComponentMap.find(name);
    if (it != mSubComponentMap.end())
    {
        if (it->second.cqs_type == "C")
        {
            vector<Component*>::iterator cit = mComponentCptrs.begin();
            for (int i=0; i < it->second.idx; ++i)
            {
                ++cit;
            }
            mComponentCptrs.erase(cit);
        }
        else if (it->second.cqs_type == "Q")
        {
            vector<Component*>::iterator cit = mComponentQptrs.begin();
            for (int i=0; i < it->second.idx; ++i)
            {
                ++cit;
            }
            mComponentQptrs.erase(cit);
        }
        else if (it->second.cqs_type == "S")
        {
            vector<Component*>::iterator cit = mComponentSignalptrs.begin();
            for (int i=0; i < it->second.idx; ++i)
            {
                ++cit;
            }
            mComponentSignalptrs.erase(cit);
        }
        else
        {
            cout << "This should not happen neither C Q or S type is set in the info" << endl;
            assert(false);
        }
    }
    else
    {
        //! @todo exception or similar instead
        cout << "The component you are trying to delete: " << name << " does not exist" << endl;
        assert(false);
    }
}

bool ComponentSystem::SubComponentStorage::have(string name)
{
    if (mSubComponentMap.count(name) > 0)
    {
        return true;
    }
    else
    {
        return false;
    }
}

void ComponentSystem::SubComponentStorage::rename(string old_name, string new_name)
{
    cout << "Trying to rename: " << old_name << " to " << new_name << endl;
    //First find the post in the map where the old name resides, copy the data stored there
    map<string, SubComponentInfo>::iterator it = mSubComponentMap.find(old_name);
    SubComponentInfo info;
    if (it != mSubComponentMap.end())
    {
        //If found erase old record
        info = it->second;
        mSubComponentMap.erase(it);

        //insert new (with new name)
        new_name = modifyName(new_name);
        cout << "new name is: " << new_name << endl;
        mSubComponentMap.insert(pair<string, SubComponentInfo>(new_name, info));

        //No change the actual component name
        //Note! we dont want to use setName here as that would create a rename loop
        //! @todo it might be a good idea to rething all of this renaming stuff, right now its prety strange (but works)
        if (info.cqs_type == "C")
        {
            mComponentCptrs[info.idx]->setName(new_name);
        }
        else if (info.cqs_type == "Q")
        {
            mComponentQptrs[info.idx]->setName(new_name);
        }
        else if (info.cqs_type == "S")
        {
            mComponentSignalptrs[info.idx]->setName(new_name);
        }
        else
        {
            cout << "Error not a C Q or S component (this should not happen" << endl;
            assert(false);
        }
    }
    else
    {
        cout << "Error now component with old_name: " << old_name << " found!" << endl;
        assert(false);
    }
}

ComponentSystem &Component::getSystemParent()
{
    return *mpSystemParent;
}

//constructor ComponentSignal
ComponentSignal::ComponentSignal(string name, double timestep) : Component(name, timestep)
{
    mTypeCQS = "S";
    mIsComponentSignal = true;
}

//constructor ComponentC
ComponentC::ComponentC(string name, double timestep) : Component(name, timestep)
{
    mTypeCQS = "C";
    mIsComponentC = true;
}

//Constructor ComponentQ
ComponentQ::ComponentQ(string name, double timestep) : Component(name, timestep)
{
    mTypeCQS = "Q";
    mIsComponentQ = true;
}

//Constructor
ComponentSystem::ComponentSystem(string name, double timestep) : Component(name, timestep)
{
    mTypeName = "ComponentSystem";
    mIsComponentSystem = true;
    mDesiredTimestep = timestep;
}

//void ComponentSystem::addComponents(vector<Component*> components)
//{
//    ///TODO: use iterator instead of idx loop
//    for (size_t idx=0; idx<components.size(); ++idx)
//    {
//        Component* comp_ptr = components[idx];
//        //! @todo add subcomponent
//        //! @todo what will happen if you change cqs type of subsystem after it has been added, maybe subsystems should be hardcoded c q or s type
//
//        if (comp_ptr->isComponentC())
//        {
//            mComponentCptrs.push_back(comp_ptr);
//        }
//        else if (comp_ptr->isComponentQ())
//        {
//            mComponentQptrs.push_back(comp_ptr);
//        }
//        else if (comp_ptr->isComponentSignal())
//        {
//            mComponentSignalptrs.push_back(comp_ptr);
//        }
//        else
//        {
//            ///TODO: use exception instead
//            cout << "Trying to add module of other type than c, q or signal" << endl;
//            assert(false);
//        }
//
//        mComponentNamesAndTypes.insert(pair<string, string>(comp_ptr->getName(), comp_ptr->getTypeName()));
//        comp_ptr->setSystemParent(*this);
//    }
//}

void ComponentSystem::addComponents(vector<Component*> components)
{
    ///TODO: use iterator instead of idx loop (not really necessary)
    for (size_t idx=0; idx<components.size(); ++idx)
    {
        mSubComponentStorage.add(components[idx]);
        components[idx]->setSystemParent(*this);
    }
}


void ComponentSystem::addComponent(Component &rComponent)
{
    vector<Component*> components;
    components.push_back(&rComponent);
    addComponents(components);
}

void ComponentSystem::addComponent(Component *pComponent)
{
    vector<Component*> components;
    components.push_back(pComponent);
    addComponents(components);
}

void ComponentSystem::renameSubComponent(string old_name, string new_name)
{
    mSubComponentStorage.rename(old_name, new_name);
}

//Component* ComponentSystem::getSubComponent(string name)
//{
//    //vector<Component*>::iterator it;
//    for (size_t s=0; s < mComponentCptrs.size(); ++s)
//    {
//        if (mComponentCptrs[s]->mName == name)
//        {
//            return mComponentCptrs[s];
//        }
//    }
//
//    for (size_t s=0; s < mComponentQptrs.size(); ++s)
//    {
//        //cout << "Comparing " << mComponentQptrs[s]->mName << " with " << name << endl;
//        if (mComponentQptrs[s]->mName == name)
//        {
//            return mComponentQptrs[s];
//        }
//    }
//
//    for (size_t s=0; s < mComponentSignalptrs.size(); ++s)
//    {
//        //cout << "Comparing " << mComponentSignalptrs[s]->mName << " with " << name << endl;
//        if (mComponentSignalptrs[s]->mName == name)
//        {
//            return mComponentSignalptrs[s];
//        }
//    }
//    cout << "Component " << name << " not found in component system!";
//    assert(false);
//    ///TODO: Cast exception if not found
//}

Component* ComponentSystem::getSubComponent(string name)
{
    return mSubComponentStorage.get(name);
}


ComponentSystem* ComponentSystem::getSubComponentSystem(string name)
{
    Component* temp_component_ptr = getSubComponent(name);
    ComponentSystem* temp_compsys_ptr = dynamic_cast<ComponentSystem*>(temp_component_ptr);

    if (temp_compsys_ptr == NULL)
    {
        cout << "dynamic cast failed, maybe " << name << " is not a component system" << endl;
        assert(false);
    }

    return temp_compsys_ptr;
}


vector<string> ComponentSystem::getSubComponentNames()
{
    //! @todo for now create a vector of the component names, later maybe we should return a pointer to the real internal map
    vector<string> names;
    map<string, SubComponentStorage::SubComponentInfo>::iterator it;
    for (it = mSubComponentStorage.mSubComponentMap.begin(); it != mSubComponentStorage.mSubComponentMap.end(); ++it)
    {
        names.push_back(it->first);
    }

    return names;
}

bool  ComponentSystem::haveSubComponent(string name)
{
    return mSubComponentStorage.have(name);
}


//! Adds a node as subnode in the system
void ComponentSystem::addSubNode(Node* node_ptr)
{
    mSubNodePtrs.push_back(node_ptr);
}

//! Removes a previously added node
void ComponentSystem::removeSubNode(Node* node_ptr)
{
    vector<Node*>::iterator it;
    for (it=mSubNodePtrs.begin(); it!=mSubNodePtrs.end(); ++it)
    {
        if (*it == node_ptr)
        {
            mSubNodePtrs.erase(it);
            break;
        }
    }
    //! @todo some notification if you try to remove something that does not exist (can not check it==mSubNodePtrs.end() ) this check can be OK after an successfull erase
}

//! preAllocates log space (to speed up later access for log writing)
void ComponentSystem::preAllocateLogSpace(const double startT, const double stopT)
{
    cout << "stopT = " << stopT << ", startT = " << startT << ", mTimestep = " << mTimestep << endl;

    ///TODO: make sure this calculation is EXACTLY correct
    double dslots = ((double)(stopT-startT))/mTimestep;
    //std::cout << "dslots: " << dslots << std::endl;
    size_t needed_slots = (size_t)(dslots+0.5); //Round to nearest
    //size_t needed_slots = ((double)(stopT-startT))/mTimestep;

    //First allocate memory for own subnodes
    vector<Node*>::iterator it;
    for (it=mSubNodePtrs.begin(); it!=mSubNodePtrs.end(); ++it)
    {
        (*it)->preAllocateLogSpace(needed_slots);
    }

    ///TODO: Call allocate for subsubsystems

}

//! Tells all subnodes contained within a system to store current data in log
void ComponentSystem::logAllNodes(const double time)
{
    vector<Node*>::iterator it;
    for (it=mSubNodePtrs.begin(); it!=mSubNodePtrs.end(); ++it)
    {
        (*it)->logData(time);
    }
}

//! Adds a transparent SubSystemPort
Port* ComponentSystem::addSystemPort(const string portname)
{
    return addPort(portname, "SystemPort", "undefined_nodetype");
}

//! Set the type C, Q, or S of the subsystem
void ComponentSystem::setTypeCQS(const string cqs_type)
{
    ///TODO: should really try to figure out a better way to do this
    ///TODO: need to do erro checking, and make sure that the specified type really is valid, first and last component should be of this type (i think)
    if (cqs_type == string("C"))
    {
        mTypeCQS = "C";
        mIsComponentC = true;
        mIsComponentQ = false;
        mIsComponentSignal = false;
    }
    else if (cqs_type == string("Q"))
    {
        mTypeCQS = "Q";
        mIsComponentC = false;
        mIsComponentQ = true;
        mIsComponentSignal = false;
    }
    else if (cqs_type == string("S"))
    {
        mTypeCQS = "S";
        mIsComponentC = false;
        mIsComponentQ = false;
        mIsComponentSignal = true;
    }
    else
    {
        cout << "Error: Specified type _" << cqs_type << "_ does not exist!" << endl;
    }
}

//! Connect two ports to each other
void ComponentSystem::connect(Port &rPort1, Port &rPort2)
{
    connect(*rPort1.mpComponent, rPort1.mPortName, *rPort2.mpComponent, rPort2.mPortName);
}

//! Connect two components with specified ports to each other, pointer version
void ComponentSystem::connect(Component *pComponent1, const string portname1, Component *pComponent2, const string portname2)
{
    connect(*pComponent1, portname1, *pComponent2, portname2);
}

//! Connect two components with specified ports to each other, reference version
void ComponentSystem::connect(Component &rComponent1, const string portname1, Component &rComponent2, const string portname2)
{
    Node* pNode;
    Port* pPort1;
    Port* pPort2;

    //First some error checking

    //Check if commponents have specified ports
    if (!rComponent1.getPort(portname1, pPort1))
    {
        //raise Exception('type of port does not exist')
        cout << "rComponent1: "<< rComponent1.getName() << " does not have a port with name " << portname1 << endl;
        assert(false);
    }

    if (!rComponent2.getPort(portname2, pPort2)) //Not else if because pPort2 has to be set in getPort
    {
        //raise Exception('type of port does not exist')
        cout << "rComponent2: "<< rComponent2.getName() << " does not have a port with name " << portname2 << endl;
        assert(false);
    }

    //! @todo this will be a problem if we want to connect sensors and such
    if (pPort1->isConnected() && pPort2->isConnected())
        //Both already are connected to nodes
    {
        //Do nothing, maybe raise exception?
        cout << "Both component ports are already connected: " << rComponent1.getName() << ": " << portname1 << " and " << rComponent2.getName() << ": " << portname2 << endl;
    }
    else
    {
        //! @todo must make sure that ONLY ONE poerport can be internally connected to systemports (no sensors or anything else) to amke stuff simpler
        //! @todo No error handling nor checks are done here
        //Check if component1 is a System component containing Component2
        if (&rComponent1 == &(rComponent2.getSystemParent()))
        {
            //! @todo check so that the parent system port is a system port
            //Create an instance of the node specified in nodespecifications
            pNode = gCoreNodeFactory.CreateInstance(pPort2->getNodeType());
            //Set nodetype in the systemport (should be empty by default)
            pPort1->mNodeType = pPort2->getNodeType();
            //add node to components and parent system
            pPort2->setNode(pNode);
            pPort1->setNode(pNode);
            pNode->setPort(pPort1);
            pNode->setPort(pPort2);
            rComponent2.getSystemParent().addSubNode(pNode);    //Component1 will contain this node as subnode
            //let the ports know about each other
            pPort1->addConnectedPort(pPort2);
            pPort2->addConnectedPort(pPort1);
        }
        //Check if component2 is a System component containing Component1
        else if (&rComponent2 == &(rComponent1.getSystemParent()))
        {
            //! @todo both these checks could be boken out into subfunction as the code is the same only swapped 1 with 2
            //Create an instance of the node specified in nodespecifications
            pNode = gCoreNodeFactory.CreateInstance(pPort1->getNodeType());
            //Set nodetype in the systemport (should be empty by default)
            pPort2->mNodeType = pPort1->getNodeType();
            //add node to components and parentsystem
            pPort1->setNode(pNode);
            pPort2->setNode(pNode);
            pNode->setPort(pPort2);
            pNode->setPort(pPort1);
            rComponent1.getSystemParent().addSubNode(pNode);    //Component2 will contain this node as subnode
            //let the ports know about each other
            pPort1->addConnectedPort(pPort2);
            pPort2->addConnectedPort(pPort1);
        }
        else
        {
            //check if both ports have the same node type specified
            if (pPort1->getNodeType() != pPort2->getNodeType())
            {
                cout << "You are trying to connect a " << rComponent1.getPort(portname1).getNodeType() << " to " << rComponent2.getPort(portname2).getNodeType()  << " when connect: " << rComponent1.getName() << ": " << portname1 << " and " << rComponent2.getName() << ": " << portname2 << endl;
                cout << "raise Exception('component port nodetypes mismatch') or similar should be here" << endl;
                assert(false);
            }
            //Check so ...C-Q-C-Q-C... pattern is consistent
            else if ((pPort1->getPortType() == "PowerPort") && (pPort2->getPortType() == "PowerPort"))
            {
                if ((pPort1->mpComponent->isComponentC()) && (pPort2->mpComponent->isComponentC()))
                {
                    cout << "Both components, " << pPort1->mpComponent->getName() << " and " << pPort2->mpComponent->getName() << ", are of C-type" << endl;
                    assert(false);
                }
                else if ((pPort1->mpComponent->isComponentQ()) && (pPort2->mpComponent->isComponentQ()))
                {
                    cout << "Both components, " << pPort1->mpComponent->getName() << " and " << pPort2->mpComponent->getName() << ", are of Q-type" << endl;
                    assert(false);
                }
            }

            //! @todo this maybe should be checked every time not only if same level, with some modification as i can connect to myself aswell
            //Check so that both systems to connect have been added to this system
            if ((&rComponent1.getSystemParent() != (Component*)this) && ((&rComponent1.getSystemParent() != (Component*)this)) )
            {
                cout << "The two components, "<< rComponent1.getName() << " and " << rComponent2.getName() << ", "<< " to be connected are not contained within the connecting system" << endl;
                assert(false);
            }

            //Check if One of the ports already is connected to a node
            if (pPort1->isConnected() || pPort2->isConnected())
            {
                //If rComponent1 is connected to a node
                if (pPort1->isConnected())
                {
                    pNode = pPort1->getNodePtr();
                    // Check so the ports can be connected
                    if (!connectionOK(pNode, pPort1, pPort2))
                    {
                        cout << "Problem occured at connection" << rComponent1.getName() << " and " << rComponent2.getName() << endl;
                        assert(false);
                    }
                    else
                    {
                        //Set node in both components ports and add it to the parent system component
                        pPort2->setNode(pNode);

                        //Add port pointers to node
                        pNode->setPort(pPort2);

                        //let the ports know about each other
                        pPort1->addConnectedPort(pPort2);
                        pPort2->addConnectedPort(pPort1);

                    }
                }
                //else rComponent2 is connected to a node
                else
                {
                    pNode = pPort2->getNodePtr();
                    // Check so the ports can be connected
                    if (!connectionOK(pNode, pPort1, pPort2))
                    {
                        cout << "Problem occured at connection" << rComponent1.getName() << " and " << rComponent2.getName() << endl;
                        assert(false);
                    }
                    else
                    {
                        //Set node in both components ports and add it to the parent system component
                        pPort1->setNode(pNode);

                        //Add port pointers to node
                        pNode->setPort(pPort1);

                        //let the ports know about each other
                        pPort1->addConnectedPort(pPort2);
                        pPort2->addConnectedPort(pPort1);
                    }
                }
            }
            //else None of the components are connected
            else
            {
                //Create an instance of the node specified in nodespecifications
                pNode = gCoreNodeFactory.CreateInstance(pPort1->getNodeType());
                cout << "Created NodeType: " << pNode->getNodeType() << endl;
                // Check so the ports can be connected
                if (!connectionOK(pNode, pPort1, pPort2))
                {
                    cout << "Problem occured at connection" << rComponent1.getName() << " and " << rComponent2.getName() << endl;
                    assert(false);
                }
                //rComponent1.getSystemparent().addSubNode(pNode); //doesnt work getSystemparent returns Component , addSubNode is in ComponentSystem
                this->addSubNode(pNode);

                //Set node in both components ports and add it to the parent system component
                pPort1->setNode(pNode);
                pPort2->setNode(pNode);

                //Add port pointers to node
                pNode->setPort(pPort1);
                pNode->setPort(pPort2);

                //let the ports know about each other
                pPort1->addConnectedPort(pPort2);
                pPort2->addConnectedPort(pPort1);
            }
        }
        cout << "Connected " << rComponent1.getName() << ": " << portname1 << " with " << rComponent2.getName() << ": " << portname2 << " sucessfully" << endl;
    }
}



bool ComponentSystem::connectionOK(Node *pNode, Port *pPort1, Port *pPort2)
{
    size_t n_ReadPorts = 0;
    size_t n_WritePorts = 0;
    size_t n_PowerPorts = 0;

    vector<Port*>::iterator it;

    //Count the different kind of ports in the node
    for (it=(*pNode).mPortPtrs.begin(); it!=(*pNode).mPortPtrs.end(); ++it)
    {
        if ((*it)->getPortType() == "ReadPort")
        {
            n_ReadPorts += 1;
        }
        if ((*it)->getPortType() == "WritePort")
        {
            n_WritePorts += 1;
        }
        if ((*it)->getPortType() == "PowerPort")
        {
            n_PowerPorts += 1;
        }
    }
    //Check the kind of ports in the components subjected for connection
    //                                 This checks that rPort1 is not already connected to pNode
    if (((pPort1->getPortType() == "ReadPort") && !(pNode->connectedToPort(pPort1))) || ((pPort2->getPortType() == "ReadPort") && !(pNode->connectedToPort(pPort2))))
    {
        n_ReadPorts += 1;
    }
    if (((pPort1->getPortType() == "WritePort") && !(pNode->connectedToPort(pPort1))) || ((pPort2->getPortType() == "WritePort") && !(pNode->connectedToPort(pPort2))))
    {
        n_WritePorts += 1;
    }
    if (((pPort1->getPortType() == "PowerPort") && !(pNode->connectedToPort(pPort1))) || ((pPort2->getPortType() == "PowerPort") && !(pNode->connectedToPort(pPort2))))
    {
        n_PowerPorts += 1;
    }
    //Check if there are some problems with the connection
    if (n_PowerPorts > 2)
    {
        cout << "Trying to connect more than two PowerPorts to same node" << endl;
        assert(false);
    }
    if (n_WritePorts > 1)
    {
        cout << "Trying to connect more than one WritePort to same node" << endl;
        assert(false);
    }
    if ((n_PowerPorts > 0) && (n_WritePorts > 0))
    {
        cout << "Trying to connect WritePort and PowerPort to same node" << endl;
        assert(false);
    }
//    if ((n_PowerPorts == 0) && (n_WritePorts == 0))
//    {
//        cout << "Trying to connect only ReadPorts" << endl;
//        assert(false);
//    }
    //It seems to be OK!
    return true;
}

//! Disconnects two ports and remove node if no one is using it any more
//! @todo whay about system ports they are somewaht speciall
void ComponentSystem::disconnect(Port *pPort1, Port *pPort2)
{
    //! @todo some simple error handling (are the ports really connected and such)
    cout << "disconnecting " << pPort1->mpComponent->getName() << " " << pPort1->getPortName() << "  and  " << pPort2->mpComponent->getName() << " " << pPort2->getPortName() << endl;

    Node* node_ptr = pPort1->getNodePtr();
    cout << "nPorts in node: " << node_ptr->mPortPtrs.size() << endl;

    //Remove the port pointer from the node and clear the port if it is not used by someone else

    //If we are only connected to one other port then clear the port and remove the pointer from the node
    //! @todo we assume that the other port is the one we are disconnecting (should be true but check may be a good idea)
    if (pPort1->mConnectedPorts.size() <= 1)
    {
        node_ptr->removePort(pPort1);
        pPort1->clearConnection();
    }
    else
    {
        pPort1->eraseConnectedPort(pPort2);
    }
    cout << "nPorts in node after remove 1: " << node_ptr->mPortPtrs.size() << endl;

    if (pPort2->mConnectedPorts.size() <= 1)
    {
        node_ptr->removePort(pPort2);
        pPort2->clearConnection();
    }
    else
    {
        pPort2->eraseConnectedPort(pPort1);
    }
    cout << "nPorts in node after remove 2: " << node_ptr->mPortPtrs.size() << endl;


    //If no more connections exist, remove the entier node and free the memory
    if (node_ptr->mPortPtrs.size() == 0)
    {
        cout << "No more connections to the node exists, deleteing the node" << endl;
        removeSubNode(node_ptr);
        delete node_ptr;
        //! @todo maybe need to let the factory remove it insted of manually, in case of user supplied external nodes
    }
}

void ComponentSystem::setDesiredTimestep(const double timestep) // FIPPLAR MED NU, be
{
    mDesiredTimestep = timestep;
    setTimestep(timestep);
}

//void ComponentSystem::setTimestep(const double timestep) // FIPPLAR MED NU, be
//{
//    mTimestep = timestep;
//
//    for (size_t s=0; s < mComponentSignalptrs.size(); ++s)
//    {
//        if (!(mComponentSignalptrs[s]->isComponentSystem()))
//        {
//            mComponentSignalptrs[s]->setTimestep(timestep);
//        }
//    }
//
//    //C components
//    for (size_t c=0; c < mComponentCptrs.size(); ++c)
//    {
//        if (!(mComponentCptrs[c]->isComponentSystem()))
//        {
//            mComponentCptrs[c]->setTimestep(timestep);
//        }
//    }
//
//    //Q components
//    for (size_t q=0; q < mComponentQptrs.size(); ++q)
//    {
//        if (!(mComponentQptrs[q]->isComponentSystem()))
//        {
//            mComponentQptrs[q]->setTimestep(timestep);
//        }
//    }
//}

void ComponentSystem::setTimestep(const double timestep) // FIPPLAR MED NU, be
{
    mTimestep = timestep;

    for (size_t s=0; s < mSubComponentStorage.mComponentSignalptrs.size(); ++s)
    {
        if (!(mSubComponentStorage.mComponentSignalptrs[s]->isComponentSystem()))
        {
            mSubComponentStorage.mComponentSignalptrs[s]->setTimestep(timestep);
        }
    }

    //C components
    for (size_t c=0; c < mSubComponentStorage.mComponentCptrs.size(); ++c)
    {
        if (!(mSubComponentStorage.mComponentCptrs[c]->isComponentSystem()))
        {
            mSubComponentStorage.mComponentCptrs[c]->setTimestep(timestep);
        }
    }

    //Q components
    for (size_t q=0; q < mSubComponentStorage.mComponentQptrs.size(); ++q)
    {
        if (!(mSubComponentStorage.mComponentQptrs[q]->isComponentSystem()))
        {
            mSubComponentStorage.mComponentQptrs[q]->setTimestep(timestep);
        }
    }
}

void ComponentSystem::adjustTimestep(double timestep, vector<Component*> componentPtrs)
{
    mTimestep = timestep;

    for (size_t c=0; c < componentPtrs.size(); ++c)
    {
        if (componentPtrs[c]->isComponentSystem())
        {
            double subTs = componentPtrs[c]->mDesiredTimestep;
//cout << componentPtrs[c]->mName << ", mTimestep: "<< componentPtrs[c]->mTimestep << endl;

            //If a subsystem's timestep is larger than this sytem's
            //timestep change it to this system's timestep
            if ((subTs > timestep) || (subTs < -0.0))
            {
                subTs = timestep;
            }
            //Check that subRs is a multiple of timestep
            else// if ((timestep/subTs - floor(timestep/subTs)) > 0.00001*subTs)
            {
                //subTs should get the nearest multiple of timestep as possible,
                subTs = timestep/floor(timestep/subTs+0.5);
            }
            componentPtrs[c]->setTimestep(subTs);
//cout << componentPtrs[c]->mName << ", subTs: "<< subTs << endl;
        }
        else
        {
            componentPtrs[c]->setTimestep(timestep);
//cout << componentPtrs[c]->mName << ", timestep: "<< timestep << endl;
        }
    }
}

////! Initializes a system component and all its contained components, also allocates log data memory
//void ComponentSystem::initialize(const double startT, const double stopT)
//{
//    //preAllocate local logspace
//    preAllocateLogSpace(startT, stopT);
//
//    adjustTimestep(mTimestep, mComponentSignalptrs);
//    adjustTimestep(mTimestep, mComponentCptrs);
//    adjustTimestep(mTimestep, mComponentQptrs);
//
//    //Init
//    //Signal components
//    for (size_t s=0; s < mComponentSignalptrs.size(); ++s)
//    {
//        if (mComponentSignalptrs[s]->isComponentSystem())
//        {
//            mComponentSignalptrs[s]->initialize(startT, stopT);
//        }
//        else
//        {
//            mComponentSignalptrs[s]->initialize();
//        }
//    }
//
//    //C components
//    for (size_t c=0; c < mComponentCptrs.size(); ++c)
//    {
//        if (mComponentCptrs[c]->isComponentSystem())
//        {
//            mComponentCptrs[c]->initialize(startT, stopT);
//        }
//        else
//        {
//            mComponentCptrs[c]->initialize();
//        }
//    }
//
//    //Q components
//    for (size_t q=0; q < mComponentQptrs.size(); ++q)
//    {
//        if (mComponentQptrs[q]->isComponentSystem())
//        {
//            mComponentQptrs[q]->initialize(startT,stopT);
//        }
//        else
//        {
//            mComponentQptrs[q]->initialize();
//        }
//
//    }
//}

//! Initializes a system component and all its contained components, also allocates log data memory
void ComponentSystem::initialize(const double startT, const double stopT)
{
    //preAllocate local logspace
    preAllocateLogSpace(startT, stopT);

    adjustTimestep(mTimestep, mSubComponentStorage.mComponentSignalptrs);
    adjustTimestep(mTimestep, mSubComponentStorage.mComponentCptrs);
    adjustTimestep(mTimestep, mSubComponentStorage.mComponentQptrs);

    //Init
    //Signal components
    for (size_t s=0; s < mSubComponentStorage.mComponentSignalptrs.size(); ++s)
    {
        if (mSubComponentStorage.mComponentSignalptrs[s]->isComponentSystem())
        {
            mSubComponentStorage.mComponentSignalptrs[s]->initialize(startT, stopT);
        }
        else
        {
            mSubComponentStorage.mComponentSignalptrs[s]->initialize();
        }
    }

    //C components
    for (size_t c=0; c < mSubComponentStorage.mComponentCptrs.size(); ++c)
    {
        if (mSubComponentStorage.mComponentCptrs[c]->isComponentSystem())
        {
            mSubComponentStorage.mComponentCptrs[c]->initialize(startT, stopT);
        }
        else
        {
            mSubComponentStorage.mComponentCptrs[c]->initialize();
        }
    }

    //Q components
    for (size_t q=0; q < mSubComponentStorage.mComponentQptrs.size(); ++q)
    {
        if (mSubComponentStorage.mComponentQptrs[q]->isComponentSystem())
        {
            mSubComponentStorage.mComponentQptrs[q]->initialize(startT,stopT);
        }
        else
        {
            mSubComponentStorage.mComponentQptrs[q]->initialize();
        }

    }
}

//! The system component version of simulate
void ComponentSystem::simulate(const double startT, const double stopT)
{
    mTime = startT;

    //Simulate
    double stopTsafe = stopT - this->mTimestep/2.0; //minus halv a timestep is here to ensure that no numerical issues occure

    while (mTime < stopTsafe)
    {
        //cout << this->getName() << ": starT: " << startT  << " stopT: " << stopT << " mTime: " << mTime << endl;
//        if (mTime > stopT-0.01)
//        {
//            //debug output for time in the last 0.01 second
//            cout << this->getName() << " time: " << mTime << " stopT: " << stopT << endl;
//        }

        logAllNodes(mTime);

        ///TODO: maybe use iterators instead
        //Signal components
        for (size_t s=0; s < mSubComponentStorage.mComponentSignalptrs.size(); ++s)
        {
            mSubComponentStorage.mComponentSignalptrs[s]->simulate(mTime, mTime+mTimestep);
        }

        //C components
        for (size_t c=0; c < mSubComponentStorage.mComponentCptrs.size(); ++c)
        {
            mSubComponentStorage.mComponentCptrs[c]->simulate(mTime, mTime+mTimestep);
        }

        //Q components
        for (size_t q=0; q < mSubComponentStorage.mComponentQptrs.size(); ++q)
        {
            mSubComponentStorage.mComponentQptrs[q]->simulate(mTime, mTime+mTimestep);
        }

        mTime += mTimestep;
    }
}

ComponentFactory gCoreComponentFactory;
DLLIMPORTEXPORT ComponentFactory* getCoreComponentFactoryPtr()
{
    return &gCoreComponentFactory;
}
