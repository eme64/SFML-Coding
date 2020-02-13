
#include "FastNoise/FastNoise.h"
//https://github.com/Auburns/FastNoise/wiki


#ifndef EVP_NOISE_HPP
#define EVP_NOISE_HPP

namespace evp {

struct DetailNoise {
   std::vector<FastNoise> noiseGen;
   DetailNoise(int &seed_, float baseFreq = 0.007, int depth=5) {
      noiseGen.resize(depth);
      for(int i=0; i<noiseGen.size(); i++) {
	 noiseGen[i].SetNoiseType(FastNoise::SimplexFractal);
	 noiseGen[i].SetSeed(seed_++);
	 noiseGen[i].SetFrequency(baseFreq*std::pow(2.0,i));
      }
   }
   inline float get(float x,float y) {
      float h0 = 0;
      for(int i=0; i<noiseGen.size(); i++) {
         h0 += (1.0+noiseGen[i].GetNoise(x,y))*0.5*std::pow(0.5,i);
      }
      return h0 * 0.5;  
   }
};

} // namespace evp

#endif //EVP_NOISE_HPP
