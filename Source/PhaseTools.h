#pragma once
#include "Alignment.h"
namespace phasetwin {
// Stereo-linked second-order all-pass: unity steady-state magnitude.
class PhaseRotator {
    struct State {double x1=0,x2=0,y1=0,y2=0;};std::array<State,2> state{};
    double rate=48000,a1=0,a2=0,target1=0,target2=0,wet=0,targetWet=0,smooth=0;
public:
    void prepare(double sr){rate=std::isfinite(sr) && sr>=8000?sr:48000;state={};wet=targetWet=0;smooth=1-std::exp(-1/(rate*.02));configure(false,80,.707f);a1=target1;a2=target2;}
    void configure(bool enabled,float frequency,float q){const double f=std::isfinite(frequency)?std::clamp(double(frequency),20.0,std::min(2000.0,rate*.4)):80;
        const double quality=std::isfinite(q)?std::clamp(double(q),.2,2.0):.707,w=2*3.141592653589793*f/rate,alpha=std::sin(w)/(2*quality),den=1+alpha;target1=-2*std::cos(w)/den;target2=(1-alpha)/den;targetWet=enabled?1:0;}
    bool isSettling()const{return std::abs(wet-targetWet)>1e-5 || std::abs(a1-target1)>1e-5 || std::abs(a2-target2)>1e-5;}
    std::array<float,2> process(const std::array<float,2>& input){a1+=smooth*(target1-a1);a2+=smooth*(target2-a2);wet+=smooth*(targetWet-wet);auto out=input;
        for(int ch=0;ch<2;++ch){auto& s=state[ch];const double x=std::isfinite(input[ch])?input[ch]:0,y=a2*x+a1*s.x1+s.x2-a1*s.y1-a2*s.y2;s.x2=s.x1;s.x1=x;s.y2=s.y1;s.y1=y;out[ch]=float(x+wet*(y-x));}return out;}
};
constexpr int spectralBands=64;
struct SpectralFrame {std::array<float,spectralBands> before{},after{},confidence{},levelDb{};double sampleRate=48000;};
inline bool phaseCurveConnects(float previous,float current,float previousSupport,float currentSupport){return std::isfinite(previous) && std::isfinite(current) && previousSupport>=.25f && currentSupport>=.25f && std::abs(current-previous)<=180;}
class SpectralAnalyzer {
    std::vector<std::complex<double>> a,b,pa,pb;
public:
    SpectralAnalyzer():a(frameSize),b(frameSize),pa(frameSize),pb(frameSize){}
    SpectralFrame analyse(const float* target,const float* reference,const float* corrected,const float* matched,double rate){SpectralFrame out;out.sampleRate=rate;out.levelDb.fill(-100);
        for(int n=0;n<frameSize;++n){const double window=.5-.5*std::cos(2*3.141592653589793*n/(frameSize-1));a[n]=target[n]*window;b[n]=reference[n]*window;pa[n]=corrected[n]*window;pb[n]=matched[n]*window;}fft(a,false);fft(b,false);fft(pa,false);fft(pb,false);
        double maximum=0,postMaximum=0;for(int n=1;n<frameSize/2;++n){maximum=std::max(maximum,std::abs(a[n])*std::abs(b[n]));postMaximum=std::max(postMaximum,std::abs(pa[n])*std::abs(pb[n]));}
        for(int band=0;band<spectralBands;++band){const double lo=20*std::pow(1000.0,double(band)/spectralBands),hi=20*std::pow(1000.0,double(band+1)/spectralBands);
            const int begin=std::max(1,int(std::ceil(lo*frameSize/rate))),end=std::min(frameSize/2,int(std::ceil(hi*frameSize/rate)));std::complex<double> pre{},post{};double power=0,postPower=0;
            for(int n=begin;n<end;++n){pre+=a[n]*std::conj(b[n]);post+=pa[n]*std::conj(pb[n]);power+=std::abs(a[n])*std::abs(b[n]);postPower+=std::abs(pa[n])*std::abs(pb[n]);}
            const int bins=std::max(1,end-begin);if(maximum>1e-12)out.levelDb[band]=float(10*std::log10(std::max(1e-10,power/(maximum*bins))));
            if(power>maximum*.001*bins && postPower>postMaximum*.001*bins && std::abs(pre)>1e-12 && std::abs(post)>1e-12){out.before[band]=float(std::arg(pre)*180/3.141592653589793);out.after[band]=float(std::arg(post)*180/3.141592653589793);out.confidence[band]=float(std::clamp(std::min(std::abs(pre)/power,std::abs(post)/postPower),0.0,1.0));}
        }return out;
    }
};
}
