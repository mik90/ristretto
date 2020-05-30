#pragma once

#include "fstext/fstext-lib.h"
#include "lat/lattice-functions.h"

namespace kaldi {
std::string LatticeToString(const Lattice& lat, const fst::SymbolTable& word_syms);
std::string GetTimeString(int32 t_beg, int32 t_end, BaseFloat time_unit);
int32 GetLatticeTimeSpan(const Lattice& lat);
std::string LatticeToString(const CompactLattice& clat, const fst::SymbolTable& word_syms);
} // namespace kaldi