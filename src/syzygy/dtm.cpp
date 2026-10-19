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


#include "dtm.h"

#include <algorithm>
#include <climits>

#include "../misc.h"
#include "../movegen.h"
#include "../position.h"
#include "../search.h"
#include "../types.h"

#ifdef USE_CHESSTB_DTM
extern "C" {
int ctb_dtm_init(const char* path);
int ctb_dtm_probe(const char* fen, int* wdl, int* dtm);
}
#endif

namespace Stockfish::DTM {

int MaxPieces = 0;

void init(const std::string& path) {
#ifdef USE_CHESSTB_DTM
    MaxPieces = path.empty() || path == "<empty>" ? 0 : ctb_dtm_init(path.c_str());
    if (MaxPieces)
        sync_cout << "info string Found DTM tables up to " << MaxPieces << " pieces" << sync_endl;
#else
    (void) path;
#endif
}

namespace {

// Result for the side to move: wdl 1 win, 0 draw, -1 loss, in `plies` plies.
struct Result {
    int wdl, plies;
};

bool probe(Position& pos, Result& r) {

    if (MoveList<ALL>(pos).size() == 0)
    {
        r = {pos.checkers() ? -1 : 0, 0};
        return true;
    }
    if (pos.count<ALL_PIECES>() == 2)
    {
        r = {0, 0};
        return true;
    }
#ifdef USE_CHESSTB_DTM
    int wdl, dtm;
    if (!ctb_dtm_probe(pos.fen().c_str(), &wdl, &dtm))
        return false;
    r = {wdl > 0 ? 1 : wdl < 0 ? -1 : 0, dtm};
    return true;
#else
    return false;
#endif
}

}  // namespace

bool rank_root_moves(Position& pos, Search::RootMoves& rootMoves, bool rule50) {

    if (!MaxPieces || rootMoves.empty() || pos.count<ALL_PIECES>() > MaxPieces
        || pos.can_castle(ANY_CASTLING))
        return false;

    const int          r50 = pos.rule50_count();
    std::vector<int>   rank(rootMoves.size());
    std::vector<Value> score(rootMoves.size());
    std::vector<bool>  known(rootMoves.size(), true);
    bool               anyUnknown = false;
    int                best       = INT_MIN;

    for (usize i = 0; i < rootMoves.size(); ++i)
    {
        const Move m       = rootMoves[i].pv[0];
        const bool zeroing = pos.capture(m) || type_of(pos.moved_piece(m)) == PAWN;
        StateInfo  st;
        Result     r;

        pos.do_move(m, st, nullptr);
        const bool ok = probe(pos, r);
        pos.undo_move(m);

        if (!ok)
            return false;

        // The side that mates does so within r.plies plies against any defence, so the
        // result holds under the 50-move rule if the clock cannot reach 100 before.
        const int plies = r.plies + 1;
        if (r.wdl != 0 && (plies >= MAX_PLY || (rule50 && (zeroing ? 0 : r50 + 1) + r.plies > 100)))
        {
            known[i]   = false;
            anyUnknown = true;
            continue;
        }

        rank[i]  = r.wdl < 0 ? 1000 - plies : r.wdl > 0 ? -1000 + plies : 0;
        score[i] = r.wdl < 0 ? mate_in(plies) : r.wdl > 0 ? mated_in(plies) : VALUE_DRAW;
        best     = std::max(best, rank[i]);
    }

    // A move without a usable result can only be set aside when a known move wins
    if (anyUnknown && best <= 0)
        return false;

    for (usize i = 0; i < rootMoves.size(); ++i)
    {
        rootMoves[i].dtmExact = known[i];
        rootMoves[i].dtmPv.clear();
        rootMoves[i].tbRank = known[i] ? rank[i] : -2000;
        if (known[i])
            rootMoves[i].tbScore = score[i];
    }

    std::stable_sort(
      rootMoves.begin(), rootMoves.end(),
      [](const Search::RootMove& a, const Search::RootMove& b) { return a.tbRank > b.tbRank; });

    return true;
}

void fill_pv(Position& pos, Search::RootMove& rootMove) {

    std::vector<Move>      line{rootMove.pv[0]};
    std::vector<StateInfo> st(MAX_PLY + 1);
    Result                 cur{0, 0};
    bool                   mated = false;

    pos.do_move(line[0], st[0], nullptr);

    while (int(line.size()) < MAX_PLY && probe(pos, cur) && cur.wdl != 0)
    {
        if (cur.plies == 0)
        {
            mated = cur.wdl < 0;
            break;
        }

        // The tables are exact: some reply leads to the opponent's result one ply sooner
        Move next = Move::none();
        for (const auto& m : MoveList<ALL>(pos))
        {
            StateInfo s;
            Result    r;
            pos.do_move(m, s, nullptr);
            const bool ok = probe(pos, r);
            pos.undo_move(m);
            if (ok && r.wdl == -cur.wdl && r.plies == cur.plies - 1)
            {
                next = m;
                break;
            }
        }

        if (next == Move::none())
            break;

        line.push_back(next);
        pos.do_move(next, st[line.size() - 1], nullptr);
    }

    for (usize i = line.size(); i > 0; --i)
        pos.undo_move(line[i - 1]);

    if (mated)
        rootMove.dtmPv = line;
}

}  // namespace Stockfish::DTM
