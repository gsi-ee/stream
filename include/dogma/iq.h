#include <cstdint>
#include <utility>
#include <vector>

namespace iqtdc {

struct hists : std::vector<uint64_t> {
  int num_chans, num_itimes, num_atans;
  inline hists(int num_atans, int num_itimes, int num_chans)
      : num_chans(num_chans), num_itimes(num_itimes), num_atans(num_atans) {
    resize(num_atans, num_itimes, num_chans);
  }

  inline void resize(int num_atans, int num_itimes, int num_chans) {
    this->num_chans = num_chans;
    this->num_itimes = num_itimes;
    this->num_atans = num_atans;
    std::vector<uint64_t>::resize(
        (num_chans * num_itimes * num_atans + 63) / 64, 0);
  }
  inline void set(int i) { (*this)[i / 64] |= uint64_t(1) << (i % 64); }
  inline int get(int i) const {
    return !!((*this)[i / 64] & (uint64_t(1) << (i % 64)));
  }

  inline void set(int c, int i, int a) {
    set(num_itimes * num_atans * c + num_atans * i + a);
  }
  inline bool get(int c, int i, int a) const {
    return get(num_itimes * num_atans * c + num_atans * i + a);
  }
};

struct calib {
  hists h;
  std::vector<std::pair<int, int>>
      gaps; // 1st: gap center, 2nd: rotation number

  inline calib(int num_atans = 0, int num_itimes = 0, int num_chans = 1)
      : h(num_atans, num_itimes, num_chans), gaps(num_chans * num_itimes) {}
  void resize(int num_atans = 0, int num_itimes = 0, int num_chans = 1) {
    gaps.resize(num_chans * num_itimes);
    h.resize(num_atans, num_itimes, num_chans);
  }
  inline bool empty() const { return h.empty(); }

  inline int compute_gap(int chan, int itime) const {
    int gapl = -1, gapr = -1, best_l = -1, best_r = 0;
    for (int i = 0; i < h.num_atans * 2; ++i) {
      if (!h.get(chan, itime, i % h.num_atans)) {
        if (gapl == -1)
          gapl = i;
        else
          gapr = i;
      } else {
        if (gapr - gapl > best_r - best_l && gapl != -1) {
          best_r = gapr;
          best_l = gapl;
        }
        gapl = -1;
        if (i >= h.num_atans)
          break;
      }
    }
    return (best_r + best_l) / 2 % h.num_atans;
  }
  inline void update_gap(int chan, int itime) {
    gaps[chan * h.num_itimes + itime].first = compute_gap(chan, itime);
  }
  inline void update_rots(int chan) {
    int rot = 0;
    int last = -1;
    for (int i = 0; i < h.num_itimes; ++i) {
      if (gaps[chan * h.num_itimes + i].first < last)
        ++rot;
      gaps[chan * h.num_itimes + i].second = rot;
      last = gaps[chan * h.num_itimes + i].first;
    }
  }

  inline void set_and_update(int chan, int itime, int atan) {
    if (chan >= h.num_chans) {
      resize(h.num_atans, h.num_itimes, chan + 1);
    }
    h.set(chan, itime, atan);
    update_gap(chan, itime);
    update_rots(chan);
  }
  inline int operator()(int chan, int itime, int atan) const {
    return ((atan < gaps[chan * h.num_itimes + itime].first ? 1 : 0) +
            gaps[chan * h.num_itimes + itime].second) *
               h.num_atans +
           atan;
  }
};

} // namespace iqtdc
