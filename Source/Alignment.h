#pragma once
#include <algorithm>
#include <cmath>
#include <complex>
#include <vector>
#include <array>
namespace phasetwin {
constexpr int frameSize = 8192;
enum class AnalysisReason { waiting,ok,targetSilent,referenceSilent,weakCorrelation,ambiguous,boundary,nonFinite };
struct Estimate { double lag = 0, confidence = 0; bool inverted = false, valid = false; AnalysisReason reason=AnalysisReason::waiting; double peakMargin=0; };
inline int strongestPair(const float* aL,const float* aR,const float* bL,const float* bR,int count){
    auto energy=[&](const float* p){double sum=0,squares=0;for(int i=0;i<count;++i){sum+=p[i];squares+=double(p[i])*p[i];}return std::max(0.0,squares-sum*sum/count);};
    return energy(aR)*energy(bR)>energy(aL)*energy(bL)?1:0;
}
inline void fft(std::vector<std::complex<double>>& a, bool inverse) {
    const auto n = a.size();
    for (size_t i=1,j=0;i<n;++i) { size_t bit=n>>1; for (;j&bit;bit>>=1) j^=bit; j^=bit; if(i<j) std::swap(a[i],a[j]); }
    for(size_t len=2;len<=n;len<<=1) {
        const double angle=(inverse?2:-2)*3.14159265358979323846/len;
        const std::complex<double> step(std::cos(angle),std::sin(angle));
        for(size_t i=0;i<n;i+=len) { std::complex<double> w(1,0); for(size_t j=0;j<len/2;++j) { auto u=a[i+j],v=a[i+j+len/2]*w; a[i+j]=u+v; a[i+j+len/2]=u-v; w*=step; } }
    }
    if(inverse) for(auto& x:a) x/=double(n);
}
// Positive lag means target A arrives later than reference B.
class Estimator {
    std::vector<std::complex<double>> a,b,cross;
    std::vector<double> ea,eb,scores;
public:
    Estimator():a(frameSize*2),b(frameSize*2),cross(frameSize*2),ea(frameSize+1),eb(frameSize+1),scores(frameSize*2+1){}
    Estimate analyse(const float* target,const float* reference,int maxLag,double threshold,int exclusionRadius=8) {
        maxLag=std::clamp(maxLag,1,frameSize/2-1);
        double ma=0,mb=0; for(int i=0;i<frameSize;++i){if(!std::isfinite(target[i]) || !std::isfinite(reference[i])) return {0,0,false,false,AnalysisReason::nonFinite}; ma+=target[i];mb+=reference[i];} ma/=frameSize;mb/=frameSize;
        ea[0]=eb[0]=0;
        for(int i=0;i<frameSize;++i){double x=target[i]-ma,y=reference[i]-mb;a[i]=x;b[i]=y;ea[i+1]=ea[i]+x*x;eb[i+1]=eb[i]+y*y;}
        std::fill(a.begin()+frameSize,a.end(),0);std::fill(b.begin()+frameSize,b.end(),0);
        if(ea.back()/frameSize<1e-9) return {0,0,false,false,AnalysisReason::targetSilent};
        if(eb.back()/frameSize<1e-9) return {0,0,false,false,AnalysisReason::referenceSilent};
        fft(a,false);fft(b,false);for(size_t i=0;i<a.size();++i)a[i]*=std::conj(b[i]);cross=a;fft(a,true);
        auto score=[&](int lag){int startA=std::max(0,lag),startB=std::max(0,-lag),count=frameSize-std::abs(lag);double den=std::sqrt((ea[startA+count]-ea[startA])*(eb[startB+count]-eb[startB]));return den>1e-12 ? a[lag>=0?lag:int(a.size())+lag].real()/den : 0.0;};
        int best=0;double peak=0;
        for(int lag=-maxLag;lag<=maxLag;++lag){auto s=score(lag);scores[lag+maxLag]=s;if(std::abs(s)>peak){peak=std::abs(s);best=lag;}}
        double second=0;for(int lag=-maxLag;lag<=maxLag;++lag)if(std::abs(lag-best)>std::max(8,exclusionRadius))second=std::max(second,std::abs(scores[lag+maxLag]));
        double fraction=0;
        if(std::abs(best)<maxLag){double l=std::abs(scores[best-1+maxLag]),c=peak,r=std::abs(scores[best+1+maxLag]),den=l-2*c+r;if(std::abs(den)>1e-12)fraction=std::clamp(0.5*(l-r)/den,-0.5,0.5);}
        // Ambiguous periodic peaks and boundary peaks cannot be safely corrected.
        const auto reason=peak<threshold?AnalysisReason::weakCorrelation:std::abs(best)==maxLag?AnalysisReason::boundary:peak-second<=0.03?AnalysisReason::ambiguous:AnalysisReason::ok;
        // Refine the unique coarse NCC peak using the Fourier-interpolated
        // correlation's derivatives. This avoids parabolic peak-shape bias.
        if(reason==AnalysisReason::ok){
            constexpr double pi=3.14159265358979323846;
            const double polarity=scores[best+maxLag]<0?-1:1;
            for(int iteration=0;iteration<4;++iteration){double derivative=0,curvature=0;
                for(std::size_t bin=1;bin<cross.size()/2;++bin){
                    const double omega=2*pi*bin/cross.size(),angle=omega*(best+fraction);
                    const double c=std::cos(angle),s=std::sin(angle),re=cross[bin].real(),im=cross[bin].imag();
                    derivative+=omega*(-re*s-im*c);curvature-=omega*omega*(re*c-im*s);
                }
                // Nyquist bin is real for real inputs; it occurs once.
                derivative-=.5*pi*cross[cross.size()/2].real()*std::sin(pi*(best+fraction));
                curvature-=.5*pi*pi*cross[cross.size()/2].real()*std::cos(pi*(best+fraction));
                if(curvature*polarity>=-1e-12)break;
                const double step=std::clamp(derivative/curvature,-.25,.25);
                fraction=std::clamp(fraction-step,-.5,.5);if(std::abs(step)<1e-7)break;
            }
        }
        return {best+fraction,std::clamp(peak,0.0,1.0),scores[best+maxLag]<0,reason==AnalysisReason::ok,reason,peak-second};
    }
};
class Delay {
    static constexpr int sincTaps=64,sincPhases=1024;
    using Kernel=std::array<std::array<float,sincTaps>,sincPhases+1>;
    static const Kernel& kernel(){
        static const Kernel table=[] {
            Kernel result{};
            constexpr double pi=3.14159265358979323846;
            for(int phase=0;phase<=sincPhases;++phase){
                const double fraction=double(phase)/sincPhases;double sum=0;
                for(int tap=0;tap<sincTaps;++tap){
                    const double x=tap-31-fraction;
                    const double sinc=std::abs(x)<1e-12?1:std::sin(pi*x)/(pi*x);
                    const double window=.42+.5*std::cos(pi*x/32)+.08*std::cos(2*pi*x/32);
                    result[phase][tap]=float(sinc*window);sum+=result[phase][tap];
                }
                for(auto& coefficient:result[phase])coefficient=float(coefficient/sum);
            }
            return result;
        }();
        return table;
    }

    std::vector<float> data;size_t head=0;
public:
    void prepare(int capacity){data.assign(std::max(4,capacity),0);head=0;(void)kernel();}
    void push(float x){data[head]=x;head=(head+1)%data.size();}
    // Requires 32 samples of interpolation guard on both sides. Table setup
    // occurs in prepare(), never during real-time processing.
    float readBandlimited(double delay)const noexcept {
        const int whole=int(std::floor(delay));const double fraction=delay-whole;
        if(whole<32 || whole+32>=int(data.size()))return read(delay);
        auto sample=[&](int offset){return data[(head+data.size()-1-size_t(offset))%data.size()];};
        if(fraction<1e-10)return sample(whole);
        const double phase=fraction*sincPhases;const int index=int(phase);const double blend=phase-index;
        const auto& coefficients=kernel();double sum=0;
        for(int tap=0;tap<sincTaps;++tap){const double weight=coefficients[index][tap]*(1-blend)+coefficients[index+1][tap]*blend;sum+=weight*sample(whole+tap-31);}
        return float(sum);
    }
    float read(double delay)const{
        delay=std::clamp(delay,0.0,double(data.size()-4));
        const int whole=int(delay);
        const double f=delay-whole;
        auto sample=[&](int offset){return double(data[(head+data.size()-1-size_t(offset))%data.size()]);};
        // Near zero delay a future sample is unavailable; use causal linear interpolation.
        if(whole==0)return float(sample(0)*(1-f)+sample(1)*f);
        // Four-point Lagrange interpolation reduces fractional-delay HF attenuation.
        const double cMinus=-f*(f-1)*(f-2)/6;
        const double c0=(f+1)*(f-1)*(f-2)/2;
        const double c1=-(f+1)*f*(f-2)/2;
        const double c2=(f+1)*f*(f-1)/6;
        return float(cMinus*sample(whole-1)+c0*sample(whole)+c1*sample(whole+1)+c2*sample(whole+2));
    }
};
}
