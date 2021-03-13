#include <stdio.h>
#include <string>
#include <iostream>
#include "pubSysCls.h"

using namespace sFnd;
using namespace std;

extern "C" {
  int initialize();
  int setenable(int iNode, bool newstate);
  int getstatus(int iNode);
  void shutdown();
}
  
// Global
SysManager myMgr;  


// Setup connections to SC-HUB
// return 0 for success, -1 for failure
int initialize()
{
  size_t portCount = 0;
  std::vector<std::string> comHubPorts;

  //Create the SysManager object. This object will coordinate actions among various ports
  // and within nodes. In this example we use this object to setup and open our port.

  //This will try to open the port. If there is an error/exception during the port opening,
  //the code will jump to the catch loop where detailed information regarding the error will be displayed;
  //otherwise the catch loop is skipped over
  try
    { 
      SysManager::FindComHubPorts(comHubPorts);
      printf("Found %d SC Hubs\n", (int)comHubPorts.size());
      for (portCount = 0; portCount < comHubPorts.size() && portCount < NET_CONTROLLER_MAX; portCount++) {
	myMgr.ComHubPort(portCount, comHubPorts[portCount].c_str()); 	//define the first SC Hub port (port 0) to be associated 
	// with COM portnum (as seen in device manager)
      }
      if (portCount > 0) {
	printf("PortsOpen(%d)\n",(int)portCount);
	myMgr.PortsOpen(portCount);				//Open the port
	for (size_t i = 0; i < portCount; i++) {
	  IPort &myPort = myMgr.Ports(i);
	  printf(" Port[%d]: state=%d, nodes=%d\n",
		 myPort.NetNumber(), myPort.OpenState(), myPort.NodeCount());
	}
      } else {
	printf("Unable to locate SC hub port\n");
	return -1;  //This terminates the main program
      }
	
    }
  catch (mnErr& theErr) {
    fprintf(stderr, "initialize: cught error: addr=%d, err=0x%0x\nmsg=%s\n",
	    theErr.TheAddr, theErr.ErrorCode, theErr.ErrorMsg);
    return -1;
  }
  catch (...) {
    fprintf(stderr, "Error generic caught\n");
    return -1;
  }
  printf("Initialize succeeded\n");
  return 0;
}

// Set enable of given node
// If this is a change from unenabled to enabled, also clear any faults
// Return 0 if success, -1 for failure
int setenable(int iNode, bool newstate) {
  try {
    static bool enabled[2]={0,0};
    if (newstate != enabled[iNode]){
      printf("Changing enable state of node %d to %d\n", iNode, newstate);
      // Get a reference to the port, to make accessing it easier
      IPort &myPort = myMgr.Ports(0);
      // Get a reference to the node, to make accessing it easier
      INode &theNode = myPort.Nodes(iNode);
      // Set enable of motor
      theNode.EnableReq(newstate);

      // Store current state
      enabled[iNode]=newstate;
    }
  } catch (mnErr& theErr) {
    fprintf(stderr, "setenable: Caught error: addr=%d, err=0x%0x\nmsg=%s\n",
	    theErr.TheAddr, theErr.ErrorCode, theErr.ErrorMsg);
    return -1;
  }
  catch (...) {
    fprintf(stderr, "Error generic caught in setenable\n");
    return -1;
  }
  return 0;
}

// Get status of node (faulted or not)
// Return 0 if ready, >0 if faulted, -1 if error
int getstatus(int iNode) {
  int status=0;

  try{
    // Get a reference to the port, to make accessing it easier
    IPort &myPort = myMgr.Ports(0);
  
    char alertList[256];
  
    // printf("Checking for Alerts in node %d: \n",iNode);
			
    // Get a reference to the node, to make accessing it easier
    INode &theNode = myPort.Nodes(iNode);

    // make sure our registers are up to date
    theNode.Status.RT.Refresh();
    theNode.Status.Alerts.Refresh();

    // Check the status register's "AlertPresent" bit
    // The bit is set true if there are alerts in the alert register
    if (!theNode.Status.RT.Value().cpm.AlertPresent) {
      ; // printf("   Node has no alerts!\n");
    }
    //Check to see if the node experienced torque saturation
    if (theNode.Status.HadTorqueSaturation()) {
      printf("      Node has experienced torque saturation since last checking\n");
      status|=1;
    }
    // get an alert register reference, check the alert register directly for alerts
    if (theNode.Status.Alerts.Value().isInAlert()) {
      // get a copy of the alert register bits and a text description of all bits set
      theNode.Status.Alerts.Value().StateStr(alertList, 256);
      printf("   Node has alerts! Alerts:\n%s\n", alertList);
      status|=2;

      // can access specific alerts using the method below
      if (theNode.Status.Alerts.Value().cpm.Common.EStopped) {
	printf("      Node is e-stopped: Clearing E-Stop\n");
	theNode.Motion.NodeStopClear();
      }
      if (theNode.Status.Alerts.Value().cpm.TrackingShutdown) {
	printf("      Node exceeded Tracking error limit\n");
      }

      // Check for alerts and clear them if possible
      theNode.Status.Alerts.Refresh();
      if (theNode.Status.Alerts.Value().isInAlert()) {
	char alertList[256];
	theNode.Status.Alerts.Value().StateStr(alertList, 256);
	printf("      Node has non-estop alerts: %s\n", alertList);
	printf("      Clearing non-serious alerts\n");
	theNode.Status.AlertsClear();
	
	// Are there still alerts?
	theNode.Status.Alerts.Refresh();
	if (theNode.Status.Alerts.Value().isInAlert()) {
	  theNode.Status.Alerts.Value().StateStr(alertList, 256);
	  printf("   Node has serious, non-clearing alerts: %s\n", alertList);
	}
	else {
	  printf("   Node %d: all alerts have been cleared\n", theNode.Info.Ex.Addr());
	}
      }
    }
  } catch (mnErr& theErr) {
    fprintf(stderr, "getstatus: Caught error: addr=%d, err=0x%0x\nmsg=%s\n",
	    theErr.TheAddr, theErr.ErrorCode, theErr.ErrorMsg);
    status = -1;
  }
  catch (...) {
    fprintf(stderr, "Error generic caught in getstatus\n");
    status = -2;
  }
  
  return status;
}

void shutdown() {
  printf("Shutting down clearpath\n");
  
  // Close down the ports
  myMgr.PortsClose();
}


// Main to test operation
// Display status of each node and then enable (clearing any errors)
int main(int argc, char *argv[]) {
  initialize();
  for (int i=0;i<2;i++) {
    printf("status %d = %d\n", i, getstatus(i));
    setenable(i,1);
  }
  shutdown();
}
