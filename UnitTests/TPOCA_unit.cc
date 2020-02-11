// 
// test basic functions of TPOCA using LHelix and PLine
//
#include "BTrk/KinKal/LHelix.hh"
#include "BTrk/KinKal/PLine.hh"
#include "BTrk/KinKal/TPOCA.hh"
#include "BTrk/KinKal/Context.hh"

#include <iostream>
#include <stdio.h>
#include <iostream>
#include <getopt.h>

#include "TH1F.h"
#include "TSystem.h"
#include "THelix.h"
#include "TPolyLine3D.h"
#include "TArrow.h"
#include "TAxis3D.h"
#include "TCanvas.h"
#include "TStyle.h"
#include "TVector3.h"
#include "TPolyLine3D.h"
#include "TPolyMarker3D.h"
#include "TLegend.h"
#include "TGraph.h"
#include "TRandom3.h"
#include "TH2F.h"
#include "TF1.h"
#include "TDirectory.h"
#include "TProfile.h"
#include "TProfile2D.h"

using namespace KinKal;
using namespace std;

void print_usage() {
  printf("Usage: TPOCA --gap f --time f --dtime f --dphi f --vprop f\n");
}

int main(int argc, char **argv) {
  int opt;
  double mom(105.0), cost(0.7), phi(0.5);
  int icharge(-1);
  double pmass(0.511), oz(100.0), ot(0.0);
  double time(8.0), dtime(20.0), dphi(5.0);
  double hlen(500.0); // half-length of the wire
  double gap(2.0); // distance between PLine and LHelix 
  double vprop(0.7);

  static struct option long_options[] = {
    {"gap",     required_argument, 0, 'g'  },
    {"time",     required_argument, 0, 't'  },
    {"dtime",     required_argument, 0, 'd'  },
    {"dphi",     required_argument, 0, 'f'  },
    {"vprop",     required_argument, 0, 'v'  }
  };

  int long_index =0;
  while ((opt = getopt_long_only(argc, argv,"", 
	  long_options, &long_index )) != -1) {
    switch (opt) {
      case 'g' : gap = atof(optarg); 
		 break;
      case 't' : time = atof(optarg);
		 break;
      case 'd' : dtime = atof(optarg);
		 break;
      case 'f' : dphi = atof(optarg);
		 break;
      case 'v' : vprop = atof(optarg);
		 break;
      default: print_usage(); 
	       exit(EXIT_FAILURE);
    }
  }
// create helix
  Context context;
  context.Bz_ = 1.0; // 1 Tesla
  Vec4 origin(0.0,0.0,oz,ot);
  float sint = sqrt(1.0-cost*cost);
  Mom4 momv(mom*sint*cos(phi),mom*sint*sin(phi),mom*cost,pmass);
  LHelix lhel(origin,momv,icharge,context);
// create pline at the specified time, separated by the specified gap
  Vec3 pos, dir;
  lhel.position(time,pos);
  lhel.direction(time,dir);
  // rotate the direction
  double lhphi = atan2(dir.Y(),dir.X());
  double pphi = lhphi + dphi;
  Vec3 pdir(cos(pphi),sin(pphi),0.0);
  double pspeed = c_*vprop; // vprop is relative to c
  Vec3 pvel = pdir*pspeed;
  // shift the position
  Vec3 perpdir(-sin(phi),cos(phi),0.0);
  Vec3 ppos = pos + gap*perpdir;
// time range;
  double ptime = time+dtime;
  TRange prange(ptime-hlen/pspeed, ptime+hlen/pspeed);
  // create the PLine
  PLine pline(ppos, pvel,time+dtime,prange);
  // create TPOCA from these
  TPOCA<LHelix,PLine> tp(lhel,pline);
  cout << "TPOCA status " << tp.status() << " doca " << tp.doca() << " dt " << tp.dt() << endl;

  return 0;
}

