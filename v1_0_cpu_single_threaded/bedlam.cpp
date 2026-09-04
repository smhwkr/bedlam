// bedlam.cpp
// Copyright (c) 2026 Sam Hawker <smhwkr@googlemail.com>. All rights reserved.

#include <windows.h>

#include <stdio.h>

#include <memory>
#include <vector>
#include <set>

struct int3
{
    int x, y, z;
};

struct ulonglong1
{
    unsigned long long int x;
};

class Piece
{
public:
    Piece(std::initializer_list<int3> coords)
    {
        this->coords = coords;
    }

    static int3 rotateA(int3 coords) { return { coords.x, coords.y, coords.z }; }
    static int3 rotateB(int3 coords) { return { coords.z, coords.x, coords.y }; }
    static int3 rotateC(int3 coords) { return { coords.y, coords.z, coords.x }; }

    static int3 rotateD(int3 coords) { return { -coords.x, -coords.y, coords.z }; }
    static int3 rotateE(int3 coords) { return { -coords.z, -coords.x, coords.y }; }
    static int3 rotateF(int3 coords) { return { -coords.y, -coords.z, coords.x }; }

    static int3 rotateG(int3 coords) { return { -coords.x, coords.y, -coords.z }; }
    static int3 rotateH(int3 coords) { return { -coords.z, coords.x, -coords.y }; }
    static int3 rotateI(int3 coords) { return { -coords.y, coords.z, -coords.x }; }

    static int3 rotateJ(int3 coords) { return { coords.x, -coords.y, -coords.z }; }
    static int3 rotateK(int3 coords) { return { coords.z, -coords.x, -coords.y }; }
    static int3 rotateL(int3 coords) { return { coords.y, -coords.z, -coords.x }; }

    static int3 rotateM(int3 coords) { return { -coords.x, coords.z, coords.y }; }
    static int3 rotateN(int3 coords) { return { -coords.y, coords.x, coords.z }; }
    static int3 rotateO(int3 coords) { return { -coords.z, coords.y, coords.x }; }

    static int3 rotateP(int3 coords) { return { coords.x, -coords.z, coords.y }; }
    static int3 rotateQ(int3 coords) { return { coords.y, -coords.x, coords.z }; }
    static int3 rotateR(int3 coords) { return { coords.z, -coords.y, coords.x }; }

    static int3 rotateS(int3 coords) { return { coords.x, coords.z, -coords.y }; }
    static int3 rotateT(int3 coords) { return { coords.y, coords.x, -coords.z }; }
    static int3 rotateU(int3 coords) { return { coords.z, coords.y, -coords.x }; }

    static int3 rotateV(int3 coords) { return { -coords.x, -coords.z, -coords.y }; }
    static int3 rotateW(int3 coords) { return { -coords.y, -coords.x, -coords.z }; }
    static int3 rotateX(int3 coords) { return { -coords.z, -coords.y, -coords.x }; }

    std::set<uint64_t> CreateOccupiedSet(int xshifts, int yshifts, int zshifts, int rots)
    {
        std::set<uint64_t> result;

        int3(*rotations[24])(int3) = {
            rotateA,
            rotateB,
            rotateC,
            rotateD,
            rotateE,
            rotateF,
            rotateG,
            rotateH,
            rotateI,
            rotateJ,
            rotateK,
            rotateL,
            rotateM,
            rotateN,
            rotateO,
            rotateP,
            rotateQ,
            rotateR,
            rotateS,
            rotateT,
            rotateU,
            rotateV,
            rotateW,
            rotateX
        };

        for (int xshift = 0; xshift < xshifts; xshift++)
        {
            for (int yshift = 0; yshift < yshifts; yshift++)
            {
                for (int zshift = 0; zshift < zshifts; zshift++)
                {
                    for (int rot = 0; rot < rots; rot++)
                    {
                        uint64_t occupied = 0;
                        for (auto& coord : this->coords)
                        {
                            int3 shifted = { coord.x + (xshift << 1), coord.y + (yshift << 1), coord.z + (zshift << 1) };
                            int3 rotated = rotations[rot](shifted);
                            int x = (rotated.x + 3) >> 1;
                            int y = (rotated.y + 3) >> 1;
                            int z = (rotated.z + 3) >> 1;
                            uint64_t mask = 1ULL << ((((z << 2) + y) << 2) + x);
                            occupied |= mask;
                        }

                        result.insert(occupied);
                    }
                }
            }
        }

        return result;
    }

    std::vector<int3> coords;
};

void PrintSolution(const ulonglong1* occupiedfromid, int level, const int* ids)
{
    int piece[13];
    uint64_t occupied[13] = {};

    for (int i = 0; i < level; i++)
    {
        piece[i] = (ids[i] >> 5) & 15;
        occupied[i] = occupiedfromid[ids[i]].x;
        printf("%i ", ids[i]);
    }

    printf("\n");

    char solution[4][4][4];
    for (int x = 0; x < 4; x++)
    {
        for (int y = 0; y < 4; y++)
        {
            for (int z = 0; z < 4; z++)
            {
                solution[z][y][x] = '-';
                uint64_t mask = 1ULL << ((((z << 2) + y) << 2) + x);
                for (int i = 0; i < level; i++)
                {
                    if (occupied[i] & mask)
                    {
                        solution[z][y][x] = 'A' + piece[i];
                    }
                }
            }
        }
    }

    for (int y = 3; y >= 0; y--)
    {
        printf("%c%c%c%c %c%c%c%c %c%c%c%c %c%c%c%c\n", solution[0][y][0], solution[0][y][1], solution[0][y][2], solution[0][y][3], solution[1][y][0], solution[1][y][1], solution[1][y][2], solution[1][y][3], solution[2][y][0], solution[2][y][1], solution[2][y][2], solution[2][y][3], solution[3][y][0], solution[3][y][1], solution[3][y][2], solution[3][y][3]);
    }

    printf("\n");
}

int main()
{
    std::set<uint64_t> occupiedset_bedlam[13] =
    {
        Piece({ { -3, -3, -3 }, { -3, -1, -3 }, { -3, 1, -3 }, { -3, 1, -1 }, { -1, 1, -3 } }).CreateOccupiedSet(3, 2, 3, 1),
        Piece({ { -3, -3, -3 }, { -1, -3, -3 }, { -1, -1, -3 }, { 1, -1, -3 }, { 1, -1, -1 } }).CreateOccupiedSet(2, 3, 3, 24),
        Piece({ { -3, -3, -3 }, { -3, -1, -3 }, { -1, -1, -3 }, { 1, -1, -3 }, { 1, -1, -1 } }).CreateOccupiedSet(2, 3, 3, 24),
        Piece({ { -3, -3, -3 }, { -1, -3, -3 }, { 1, -3, -3 }, { -1, -1, -3 }, { -1, -1, -1 } }).CreateOccupiedSet(2, 3, 3, 24),
        Piece({ { -3, -3, -3 }, { -3, -1, -3 }, { -3, 1, -3 }, { -1, 1, -3 }, { -1, 1, -1 } }).CreateOccupiedSet(3, 2, 3, 24),
        Piece({ { -3, -1, -3 }, { -1, -1, -3 }, { -1, -3, -3 }, { 1, -1, -3 }, { 1, 1, -3 } }).CreateOccupiedSet(2, 2, 4, 24),
        Piece({ { -3, -3, -3 }, { -1, -3, -3 }, { -1, -1, -3 }, { -1, -1, -1 }, { 1, -1, -1 } }).CreateOccupiedSet(2, 3, 3, 24),
        Piece({ { -3, -1, -3 }, { -1, -1, -3 }, { -1, -3, -3 }, { 1, -1, -3 }, { 1, -1, -1 } }).CreateOccupiedSet(2, 3, 3, 24),
        Piece({ { -3, -1, -3 }, { -1, -1, -3 }, { -1, -3, -3 }, { -1, -1, -1 }, { 1, -1, -3 } }).CreateOccupiedSet(2, 3, 3, 24),
        Piece({ { -3, -3, -3 }, { -1, -3, -3 }, { -1, -1, -3 }, { -1, -1, -1 }, { 1, -1, -3 } }).CreateOccupiedSet(2, 3, 3, 24),
        Piece({ { -3, -3, -3 }, { -1, -3, -3 }, { -1, -1, -3 }, { 1, -1, -3 }, { 1, 1, -3 } }).CreateOccupiedSet(2, 2, 4, 24),
        Piece({ { -3, -1, -3 }, { -1, -1, -3 }, { -1, -3, -3 }, { 1, -1, -3 }, { -1, 1, -3 } }).CreateOccupiedSet(2, 2, 4, 24),
        Piece({ { -3, -1, -3 }, { -1, -1, -3 }, { -1, -3, -3 }, { -3, -1, -1 } }).CreateOccupiedSet(3, 3, 3, 24)
    };

    std::set<uint64_t> occupiedset_abraxis[13] =
    {
        Piece({ { -3, -3, -3 }, { -3, -1, -3 }, { -3, 1, -3 }, { -1, 1, -3 }, { -3, 1, -1 } }).CreateOccupiedSet(3, 2, 3, 1),
        Piece({ { -1, -3, -3 }, { -1, -1, -3 }, { -1, 1, -3 }, { -3, -1, -3 }, { -3, -1, -1 } }).CreateOccupiedSet(3, 2, 3, 24),
        Piece({ { -3, -3, -3 }, { -3, -1, -3 }, { -3, 1, -3 }, { -1, -1, -3 }, { -3, -1, -1 } }).CreateOccupiedSet(3, 2, 3, 24),
        Piece({ { -1, -3, -3 }, { -1, -1, -3 }, { -3, -1, -3 }, { -3, 1, -3 }, { -3, -1, -1 } }).CreateOccupiedSet(3, 2, 3, 24),
        Piece({ { -3, -3, -3 }, { -3, -1, -3 }, { -1, -1, -3 }, { -1, 1, -3 }, { -3, -1, -1 } }).CreateOccupiedSet(3, 2, 3, 24),
        Piece({ { -3, -3, -3 }, { -3, -1, -3 }, { -3, 1, -3 }, { -1, -3, -3 }, { -3, -1, -1 } }).CreateOccupiedSet(3, 2, 3, 24),
        Piece({ { -3, -3, -3 }, { -3, -1, -3 }, { -3, 1, -3 }, { -1, 1, -3 }, { -3, -1, -1 } }).CreateOccupiedSet(3, 2, 3, 24),
        Piece({ { -3, -1, -3 }, { -1, -1, -3 }, { -1, 1, -3 }, { -3, -3, -1 }, { -3, -1, -1 } }).CreateOccupiedSet(3, 2, 3, 24),
        Piece({ { -1, -3, -3 }, { -1, -1, -3 }, { -3, -1, -3 }, { -3, -1, -1 }, { -3, 1, -1 } }).CreateOccupiedSet(3, 2, 3, 24),
        Piece({ { -3, -3, -3 }, { -1, -3, -3 }, { -1, -1, -3 }, { -1, 1, -3 }, { -3, -3, -1 } }).CreateOccupiedSet(3, 2, 3, 24),
        Piece({ { -3, -3, -3 }, { -3, -1, -3 }, { -3, 1, -3 }, { -1, -3, -3 }, { -3, 1, -1 } }).CreateOccupiedSet(3, 2, 3, 24),
        Piece({ { -1, -3, -3 }, { -1, -1, -3 }, { -3, -1, -3 }, { -3, 1, -3 }, { -3, 1, -1 } }).CreateOccupiedSet(3, 2, 3, 24),
        Piece({ { -3, -3, -3 }, { -3, -1, -3 }, { -3, -1, -1 }, { -1, -1, -3 } }).CreateOccupiedSet(3, 3, 3, 24)
    };

    auto& occupiedset = occupiedset_bedlam;

    ulonglong1 occupiedfromid[64 * 512] = {};
    uint64_t unsetmask = 0;
    uint64_t setmask = 1;
    for (int target = 0; target < 64; target++)
    {
        for (int piece = 0; piece < 13; piece++)
        {
            int id = (target << 9) + (piece << 5);
            for (auto& occupied : occupiedset[piece])
            {
                if (!(occupied & unsetmask) && (occupied & setmask))
                {
                    occupiedfromid[id].x = occupied;
                    id++;
                }
            }
        }
        unsetmask |= setmask;
        setmask = setmask << 1;
    }

    printf("bedlam v1.0\n");
    printf("Copyright (c) 2026 Sam Hawker <smhwkr@googlemail.com>. All rights reserved.\n\n");

    LARGE_INTEGER frequency;
    LARGE_INTEGER start, end;

    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&start);

    int solutions = 0;

    uint32_t pieces = 0x1fff;
    uint64_t occupied = 0;

    int ids[13];

    int level = 0;
    int targetn = 0;
    uint32_t piecenbit = 1;
    int piecen = 0;
    int id = 0;

    while (1)
    {
        uint64_t occupiedn = occupiedfromid[id].x;

        if (occupiedn)
        {
            if (occupied & occupiedn)
            {
                // conflicts
                id++;
                continue;
            }
            // fits
            ids[level] = id;
            if (level + 1 == 13)
            {
                // found a solution
                solutions++;
                printf("#%i\n", solutions);
                PrintSolution(occupiedfromid, level + 1, ids);
                id++;
                continue;
            }
            // descend one level
            level++;
            pieces ^= piecenbit;
            occupied |= occupiedn;
            targetn = std::countr_zero(~occupied);
            piecenbit = pieces & static_cast<uint32_t>(-static_cast<int32_t>(pieces));
            piecen = std::countr_zero(piecenbit);
            id = (targetn << 9) + (piecen << 5);
            continue;
        }
        // select a piece that has not already been used
        uint32_t temppieces = pieces & static_cast<uint32_t>(-static_cast<int32_t>(piecenbit << 1));
        if (temppieces)
        {
            // found a piece
            piecenbit = temppieces & static_cast<uint32_t>(-static_cast<int32_t>(temppieces));
            piecen = std::countr_zero(piecenbit);
            id = (targetn << 9) + (piecen << 5);
            continue;
        }
        // all pieces are invalid
        if (level > 0)
        {
            // ascend one level
            level--;
            id = ids[level];
            targetn = id >> 9;
            piecen = (id >> 5) & 15;
            piecenbit = 1U << piecen;
            occupiedn = occupiedfromid[id].x;
            pieces ^= piecenbit;
            occupied &= ~occupiedn;
            id++;
            continue;
        }
        // exhausted search space
        break;
    }

    printf("ms\n");

    QueryPerformanceCounter(&end);
    double elapsed_ms = (end.QuadPart - start.QuadPart) * 1000.0 / frequency.QuadPart;
    printf("%0.1lf\n\n", elapsed_ms);

    printf("%i solutions found\n", solutions);

    return 0;
}
