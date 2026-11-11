// SPDX-License-Identifier: BSD-3-Clause
#pragma once
#include "UnicodeGraphemeData.h"
#include <vector>
namespace conpty_unicode {
enum Property { Other, CR, LF, Control, Extend, ZWJ, RI, Prepend, SpacingMark, L, V, T, LV, LVT };
template <std::size_t N> inline unsigned Lookup(const Range (&ranges)[N], unsigned cp)
{
    std::size_t begin=0,end=N;
    while(begin<end){const auto mid=begin+(end-begin)/2;if(cp<ranges[mid].first)end=mid;else if(cp>ranges[mid].last)begin=mid+1;else return ranges[mid].value;}
    return 0;
}
inline unsigned GraphemeProperty(unsigned cp)
{
    if(cp>=0xAC00&&cp<=0xD7A3)return (cp-0xAC00)%28?LVT:LV;
    return Lookup(GraphemeRanges,cp);
}
// UAX #29 extended grapheme cluster rules, Unicode 17 (GB3 through GB999).
inline bool BreakBefore(const std::vector<unsigned>& cluster,unsigned next)
{
    if(cluster.empty())return true;
    const unsigned a=GraphemeProperty(cluster.back()),b=GraphemeProperty(next);
    if(a==CR&&b==LF)return false;
    if(a==CR||a==LF||a==Control||b==CR||b==LF||b==Control)return true;
    if(a==L&&(b==L||b==V||b==LV||b==LVT))return false;
    if((a==LV||a==V)&&(b==V||b==T))return false;
    if((a==LVT||a==T)&&b==T)return false;
    if(b==Extend||b==ZWJ||b==SpacingMark||a==Prepend)return false;
    if(Lookup(IndicConjunctRanges,next)==1){
        bool linker=false;std::size_t i=cluster.size();
        while(i){unsigned p=Lookup(IndicConjunctRanges,cluster[i-1]);if(p!=2&&p!=3)break;linker|=p==3;--i;}
        if(linker&&i&&Lookup(IndicConjunctRanges,cluster[i-1])==1)return false;
    }
    if(a==ZWJ&&Lookup(ExtendedPictographicRanges,next)){
        std::size_t i=cluster.size()-1;
        while(i&&GraphemeProperty(cluster[i-1])==Extend)--i;
        if(i&&Lookup(ExtendedPictographicRanges,cluster[i-1]))return false;
    }
    if(a==RI&&b==RI){std::size_t count=0;for(auto i=cluster.rbegin();i!=cluster.rend()&&GraphemeProperty(*i)==RI;++i)++count;if(count%2)return false;}
    return true;
}
}
