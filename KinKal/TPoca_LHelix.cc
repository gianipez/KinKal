#include "KinKal/TPoca.hh"
#include "KinKal/LHelix.hh"
#include "KinKal/TLine.hh"
#include "KinKal/PKTraj.hh"
#include <limits>
// specializations for TPoca
using namespace std;
namespace KinKal {
  // specialization between a looping helix and a line
  template<> TPoca<LHelix,TLine>::TPoca(LHelix const& lhelix, TLine const& tline, TPocaHint const& hint, float precision) : TPocaBase(precision),ktraj_(&lhelix), straj_(&tline) {
    // reset status
    reset();
    float htoca,stoca;
    // initialize the helix time using hints, if available.  If not, use the Z of the line
    if(hint.particleHint_)
      htoca = hint.particleToca_;
    else
      htoca = lhelix.ztime(tline.z0());
    // similar for line; this shouldn't matter, since the solution is linear
    if(hint.sensorHint_)
      stoca = hint.sensorToca_;
    else
      stoca = tline.t0();
    // use successive linear approximation until desired precision on DOCA is met.
    float dptoca(std::numeric_limits<float>::max()), dstoca(std::numeric_limits<float>::max());
    float doca(0.0);
    static const unsigned maxiter=100; // don't allow infinite iteration.  This should be a parameter FIXME!
    unsigned niter(0);
    // helix speed doesn't change
    float hspeed = lhelix.speed(lhelix.t0());
    Vec3 hdir;
    // iterate until change in TOCA is less than precision
    while((fabs(dptoca) > precision_ || fabs(dstoca) > precision_) && niter++ < maxiter) {
      // find helix local position and direction
      Vec3 hpos;
      lhelix.position(htoca,hpos);
      lhelix.direction(htoca,hdir);
      auto dpos = tline.pos0()-hpos;
      // dot products
      float ddot = tline.dir().Dot(hdir);
      float denom = 1.0 - ddot*ddot;
      // check for parallel)
      if(denom<1.0e-5){
	status_ = pocafailed;
	break;
      }
      float hdd = dpos.Dot(hdir);
      float ldd = dpos.Dot(tline.dir());
      // compute length from expansion point to POCA and convert to times
      dptoca = (hdd - ldd*ddot)/(denom*hspeed);
      dstoca = tline.t0() + (hdd*ddot - ldd)/(denom*tline.speed(stoca)) - stoca;
      htoca += dptoca; // helix time is iterative
      stoca += dstoca; // line time is always WRT t0, since it uses p0
      // compute DOCA
      lhelix.position(htoca,hpos);
      Vec3 lpos;
      tline.position(stoca,lpos);
      float dd2 = (hpos-lpos).Mag2();
      if(dd2 < 0.0 ){
	status_ = pocafailed;
	break;
      }
      doca = sqrt(dd2);
      // update convergence test
      if(isnan(doca)){
	status_ = pocafailed;
	break;
      }
    }
    // if successfull, finalize TPoca
    if(status_ != pocafailed){
      if(niter < maxiter)
        status_ = TPoca::converged;
      else
        status_ = TPoca::unconverged;
        // set the positions
      partPoca_.SetE(htoca);
      lhelix.position(partPoca_);
      sensPoca_.SetE(stoca);
      tline.position(sensPoca_);
      // sign doca by angular momentum projected onto difference vector
      float lsign = tline.dir().Cross(hdir).Dot(sensPoca_.Vect()-partPoca_.Vect());
      float dsign = copysign(1.0,lsign);
      doca_ = doca*dsign;

      // pre-compute some values needed for the derivative calculations
      float time = particlePoca().T();
      Vec3 vdoca, ddir;
      delta(vdoca);
      ddir = vdoca.Unit();// direction vector along D(POCA) from traj 2 to 1 (line to helix)
      float invpbar = lhelix.sign()/lhelix.pbar();
      Vec3 t1 = lhelix.momDir(KInter::theta1,time);
      Vec3 t2 = lhelix.momDir(KInter::theta2,time);
      float coseta = ddir.Dot(t1);
      float sineta = ddir.Dot(t2);

      // no t0 dependence, DOCA is purely geometric
      dDdP_[LHelix::cx_] = -dsign*ddir.x();
      dDdP_[LHelix::cy_] = -dsign*ddir.y();
      dDdP_[LHelix::phi0_] = -dsign*lhelix.rad()*lhelix.lam()*invpbar*coseta;
      dDdP_[LHelix::rad_] = dsign*sineta;
      dDdP_[LHelix::lam_] = dsign*lhelix.dphi(time)*lhelix.rad()*invpbar*coseta;

      // no spatial dependence, DT is purely temporal
      dTdP_[LHelix::t0_] = 1.0; // time is 100% correlated
      // propagate parameter covariance to variance on doca and toca
      docavar_ = ROOT::Math::Similarity(dDdP(),lhelix.params().covariance());
      tocavar_ = ROOT::Math::Similarity(dTdP(),lhelix.params().covariance());
      // dot product between directions at POCA
      Vec3 pdir, sdir;
      lhelix.direction(particleToca(),pdir);
      tline.direction(sensorToca(),sdir);
      ddot_ = pdir.Dot(sdir);
    }
  }

  // specialization between a piecewise LHelix and a line
  typedef PKTraj<LHelix> PLHELIX;
  template<> TPoca<PLHELIX,TLine>::TPoca(PLHELIX const& phelix, TLine const& tline, TPocaHint const& hint, float precision) : TPocaBase(precision), ktraj_(&phelix), straj_(&tline)  {
    // iteratively find the nearest piece, and POCA for that piece.  Start at hints if availalble, otherwise the middle
    static const unsigned maxiter=10; // don't allow infinite iteration.  This should be a parameter FIXME!
    unsigned niter=0;
    size_t oldindex= phelix.pieces().size();
    size_t index;
    if(hint.particleHint_)
      index = phelix.nearestIndex(hint.particleToca_);
    else
      index = size_t(rint(oldindex/2.0));
    status_ = converged; 
    while(status_ == converged && niter++ < maxiter && index != oldindex){
      // call down to LHelix TPoca
      // prepare for the next iteration
      LHelix const& piece = phelix.pieces()[index];
      TPoca<LHelix,TLine> tpoca(piece,tline,hint,precision);
      status_ = tpoca.status();
      if(tpoca.usable()){
	// copy over the rest of the state
	partPoca_ = tpoca.particlePoca();
	sensPoca_ = tpoca.sensorPoca();
	doca_ = tpoca.doca();
	dDdP_ = tpoca.dDdP();
	dTdP_ = tpoca.dTdP();
	docavar_ = tpoca.docaVar();
	tocavar_ = tpoca.tocaVar();
	ddot_ = tpoca.dirDot();
      }
      oldindex = index;
      index = phelix.nearestIndex(tpoca.particlePoca().T());
    }
    if(status_ == converged && niter >= maxiter) status_ = unconverged;
  }

}
