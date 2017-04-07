/* PeTe - Petri Engine exTremE
 * Copyright (C) 2011  Jonas Finnemann Jensen <jopsen@gmail.com>,
 *                     Thomas Søndersø Nielsen <primogens@gmail.com>,
 *                     Lars Kærlund Østergaard <larsko@gmail.com>,
 *                     Peter Gjøl Jensen <root@petergjoel.dk>
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "PetriNet.h"
#include "PQL/PQL.h"
#include "PQL/Contexts.h"
#include "Structures/State.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
using namespace std;

namespace PetriEngine {

    PetriNet::PetriNet(uint32_t trans, uint32_t invariants, uint32_t places)
    : _ninvariants(invariants), _ntransitions(trans), _nplaces(places),
            _transitions(_ntransitions+1),
            _invariants(_ninvariants),            
            _placeToPtrs(_nplaces+1) {

        // to avoid special cases
        _transitions[_ntransitions].inputs = _ninvariants;
        _transitions[_ntransitions].outputs = _ninvariants;
        _placeToPtrs[_nplaces] = _ntransitions;
        _initialMarking = new MarkVal[_nplaces];
    }

    PetriNet::~PetriNet() {
        delete[] _initialMarking;
    }

    int PetriNet::inArc(uint32_t place, uint32_t transition) const
    {
        assert(place < _nplaces);
        assert(transition < _ntransitions);
        
        uint32_t imin = _transitions[transition].inputs;
        uint32_t imax = _transitions[transition].outputs;
        if(imin == imax)
        {
            // NO INPUT!
            return 0;
        }
        
        for(;imin < imax; ++imin)
        {
            const Invariant& inv = _invariants[imin];
            if(inv.place == place)
            {
                return inv.inhibitor ? 0 :  _invariants[imin].tokens;
            }
        }
        return 0;
    }
    int PetriNet::outArc(uint32_t transition, uint32_t place) const
    {
        assert(place < _nplaces);
        assert(transition < _ntransitions);
        
        uint32_t imin = _transitions[transition].outputs;
        uint32_t imax = _transitions[transition+1].inputs;
        for(;imin < imax; ++imin)
        {
            if(_invariants[imin].place == place) return _invariants[imin].tokens;
        }
        return 0;   
    }
    
    bool PetriNet::deadlocked(const MarkVal* m) const {
        
        //Check that we can take from the marking
        for (size_t i = 0; i < _nplaces; i++) {
            if(i == 0 || m[i] > 0) // orphans are currently under "place 0" as a special case
            {
                uint32_t first = _placeToPtrs[i];
                uint32_t last = _placeToPtrs[i+1];
                for(;first != last; ++first)
                {
                    const TransPtr& ptr = _transitions[first];
                    uint32_t finv = ptr.inputs;
                    uint32_t linv = ptr.outputs;
                    bool allgood = true;
                    for(;finv != linv; ++finv)
                    {
                        allgood &= m[_invariants[finv].place] >= _invariants[finv].tokens; // match tokens
                        if(_invariants[finv].inhibitor)
                        {
                            if(allgood)
                            {
                                allgood = false;
                                break;
                            }
                            else
                            {
                                allgood = true;
                            }
                        }
                    }
                    
                    if(allgood) 
                    {
                        return false;
                    }
                }
            }
        }

        return true;
    }

    bool PetriNet::fireable(const MarkVal *marking, int transitionIndex)
    {
        const TransPtr& transition = _transitions[transitionIndex];
        uint32_t first = transition.inputs;
        uint32_t last = transition.outputs;

        for(;first < last; ++first){
            const Invariant& inv = _invariants[first];
            if(inv.inhibitor == (marking[inv.place] >= inv.tokens))
                return false;
        }
        return true;
    }
    

    MarkVal* PetriNet::makeInitialMarking()
    {
        MarkVal* marking = new MarkVal[_nplaces];
        memcpy(marking, _initialMarking, sizeof(MarkVal)*_nplaces);
        return marking;
    }
    
} // PetriEngine
