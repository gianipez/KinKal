#ifndef KinKal_StrawHit_hh
#define KinKal_StrawHit_hh
//
//  class representing a straw sensor measurement.  It assumes a (possibly displaced)
//  circular outer cathode locally parallel to the wire.
//  Used as part of the kinematic Kalman fit
//
#include "KinKal/WireHit.hh"
#include "KinKal/StrawXing.hh"
#include <memory>
namespace KinKal {
  template <class KTRAJ> class StrawHit : public WireHit<KTRAJ> {
    public:
      typedef PKTraj<KTRAJ> PKTRAJ;
      typedef WireHit<KTRAJ> WHIT;
      typedef THit<KTRAJ> THIT;
      typedef StrawXing<KTRAJ> STRAWXING;
      StrawHit(BField const& bfield, PKTRAJ const& pktraj, TLine const& straj, D2T const& d2t, std::shared_ptr<STRAWXING> const& sxing,float nulldoca, LRAmbig ambig=LRAmbig::null,bool active=true) :
	WireHit<KTRAJ>(bfield,pktraj,straj,d2t,std::min(nulldoca,sxing->strawMat().strawRadius())*std::min(nulldoca,sxing->strawMat().strawRadius())/3.0,ambig,active) { // clumsy FIXME!
	  THIT::dxing_ = sxing; }
      virtual float tension() const override { return 0.0; } // check against straw diameter, length, any other measurement content FIXME!
      virtual void print(std::ostream& ost=std::cout,int detail=0) const override;
      virtual ~StrawHit(){}
    private:
//      std::shared_ptr<STRAWXING> sxing_; // straw material crossing information.  Not sure if this is needed FIXME!
      // add state for longitudinal resolution, transverse resolution FIXME!
  };

  template<class KTRAJ> void StrawHit<KTRAJ>::print(std::ostream& ost, int detail) const {
    if(this->isActive())
      ost<<"Active ";
    else
      ost<<"Inactive ";
    ost << " StrawHit LRAmbig " << this-> ambig() << " ";
    WireHit<KTRAJ>::poca().print(ost,detail);
  }
}
#endif
