// C interface to the chesstb probing code (https://github.com/noobpwnftw/chesstb,
// public domain), so that Stockfish can read distance-to-mate values without
// including chesstb headers, whose type names overlap with Stockfish's.

#include "probe/probe.h"
#include "chess/position.h"

#include <cstring>
#include <memory>
#include <sstream>
#include <string>

namespace {

std::unique_ptr<Probe_Tables> tables;

}  // namespace

// chesstb calls the MSVC intrinsic __umulh on Windows; MinGW declares it but has no body.
#if defined(_WIN32) && defined(__GNUC__)
extern "C" unsigned long long __umulh(unsigned long long a, unsigned long long b) {
    return (unsigned long long) (((unsigned __int128) a * b) >> 64);
}
#endif

extern "C" {

// Loads the tables below `path` (a folder holding wdl/ and dtm/ subfolders).
// Returns the largest piece count found, 0 if none.
int ctb_dtm_init(const char* path) {
    tables.reset();
    if (!path || !*path)
        return 0;
    try
    {
        auto t = std::make_unique<Probe_Tables>();
        if (!t->init(path) || t->largest() == 0)
            return 0;
        t->set_block_cache_bytes(size_t(256) << 20);
        tables = std::move(t);
        return int(tables->largest());
    } catch (...)
    {
        return 0;
    }
}

// Probes a position given as FEN. Returns 1 and sets wdl (-2 loss .. 2 win,
// side to move) and dtm (plies to mate, the 50-move rule ignored) on success,
// 0 if the position is not in the tables or has no DTM value.
int ctb_dtm_probe(const char* fen, int* wdl, int* dtm) {
    if (!tables)
        return 0;
    try
    {
        const Position pos = Position::from_fen(fen);
        if (!pos.is_legal())
            return 0;

        std::istringstream ss(fen);
        std::string board, stm, castling, ep = "-";
        ss >> board >> stm >> castling >> ep;
        const Square epSq = ep == "-" ? SQ_END : square_from_string(ep.c_str());

        const Probe_Result r = tables->probe(pos, epSq, 0);
        if (r.status != Probe_Result::Status::OK || !r.has_dtm)
            return 0;

        switch (r.wdl)
        {
        case WDL_Entry::LOSE :         *wdl = -2; break;
        case WDL_Entry::BLESSED_LOSS : *wdl = -1; break;
        case WDL_Entry::DRAW :         *wdl = 0; break;
        case WDL_Entry::CURSED_WIN :   *wdl = 1; break;
        case WDL_Entry::WIN :          *wdl = 2; break;
        default :                      return 0;
        }
        *dtm = int(r.dtm);
        return 1;
    } catch (...)
    {
        return 0;
    }
}

}  // extern "C"
