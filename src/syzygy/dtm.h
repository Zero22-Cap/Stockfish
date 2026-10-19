/*
  Stockfish, a UCI chess playing engine derived from Glaurung 2.1
  Copyright (C) 2004-2026 The Stockfish developers (see AUTHORS file)

  Stockfish is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Stockfish is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#ifndef DTM_H_INCLUDED
#define DTM_H_INCLUDED

#include <string>
#include <vector>

namespace Stockfish {
class Position;

namespace Search {
struct RootMove;
using RootMoves = std::vector<RootMove>;
}
}

// Exact distance-to-mate from chesstb tables, used only for the root position and
// the displayed line, never inside the search. Needs a build with CHESSTB_DTM.
namespace Stockfish::DTM {

extern int MaxPieces;

void init(const std::string& path);

// If the root position is in the tables, gives every root move its exact result
// (tbScore, tbRank, dtmExact) and sorts them. Returns false and leaves the root
// moves as they are otherwise, or when the 50-move rule could change a result.
bool rank_root_moves(Position& pos, Search::RootMoves& rootMoves, bool rule50);

// Fills rootMove.dtmPv with the line to mate given by the tables.
void fill_pv(Position& pos, Search::RootMove& rootMove);

}  // namespace Stockfish::DTM

#endif  // #ifndef DTM_H_INCLUDED
