/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0.  If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright 1997 - July 2008 CWI, August 2008 - 2016 MonetDB B.V.
 */

/* author: M Kersten
 * Post-optimization of projection lists.
 */
#include "monetdb_config.h"
#include "opt_deadcode.h"
#include "opt_projectionpath.h"


// Common prefix reduction was not effective it is retained for 
// future experiments.
//#define ELIMCOMMONPREFIX

#define LOOKAHEAD 500   /* limit the lookahead for candidates */

/* locate common prefixes  in projection lists 
 * The algorithm is quadratic in the number of paths considered. */

#ifdef ELIMCOMMONPREFIX
static int
OPTprojectionPrefix(Client cntxt, MalBlkPtr mb, int prefixlength)
{
	int i, j, k, match, actions=0;
	InstrPtr p,q,r,*old;
	int limit, slimit;

	old = mb->stmt;
	limit = mb->stop;
	slimit= mb->ssize;
	if (newMalBlkStmt(mb,mb->ssize) < 0)
		return 0;
#ifdef DEBUG_OPT_PROJECTIONPATH
	mnstr_printf(cntxt->fdout,"#projectionpath find common prefix prefixlength %d\n", prefixlength);
#endif
 
	for( i = 0; i < limit; i++){
		p = old[i];
		assert(p);
		if ( getFunctionId(p) != projectionpathRef || p->argc < prefixlength) {
			pushInstruction(mb,p);
			continue;
		}
#ifdef DEBUG_OPT_PROJECTIONPATH
		mnstr_printf(cntxt->fdout,"#projectionpath candidate prefix pc %d \n", i);
		printInstruction(cntxt->fdout,mb, 0, p, LIST_MAL_ALL);
#endif
		/* we fixed a projection path of the target prefixlength
		 * Search now the remainder for at least one case where it
		 * has a common prefix of prefixlength 
		 */
		for(match = 0,  j= i+1; j < limit && j < i + LOOKAHEAD; j++) {
			q= old[j];
			if ( getFunctionId(q) != projectionpathRef || q->argc < prefixlength) 
				continue;
			for( match =0,  k = q->retc; k <  prefixlength; k++)
				match += getArg(q,k) == getArg(p,k);
			if ( match == prefixlength - q->retc )
				break;
			match = 0;
		}
		if ( match && match == prefixlength - q->retc ){
			/* at least one instruction has been found.
			 * Inject the prefex projection path and replace all use cases
			 */
#ifdef DEBUG_OPT_PROJECTIONPATH
			mnstr_printf(cntxt->fdout,"#projectionpath found common prefix pc %d \n", j);
			printInstruction(cntxt->fdout,mb, 0, p, LIST_MAL_ALL);
#endif
			/* create the factored out prefix projection */
			r = copyInstruction(p);
			r->argc = prefixlength;
			getArg(r,0) = newTmpVariable(mb, newBatType(getBatType(getArgType(mb,r,r->argc-1))));
			setVarUDFtype(mb, getArg(r,0));
			if( r->argc == 3)
				setFunctionId(r,projectionRef);
			r->typechk = TYPE_UNKNOWN;
			pushInstruction(mb,r);
#ifdef DEBUG_OPT_PROJECTIONPATH
			mnstr_printf(cntxt->fdout,"#projectionpath prefix instruction\n");
			printInstruction(cntxt->fdout,mb, 0, r, LIST_MAL_ALL);
#endif

			/* patch all instructions with same prefix. */
			for( ; j < limit; j++) {
				q= old[j];
				if ( getFunctionId(q) != projectionpathRef || q->argc < prefixlength) 
					continue;
				for( match =0,  k = r->retc; k < r->argc; k++)
					match += getArg(q,k) == getArg(r,k);
				if (match &&  match == prefixlength - r->retc ){
					actions++;
#ifdef DEBUG_OPT_PROJECTIONPATH
					mnstr_printf(cntxt->fdout,"#projectionpath before:");
					printInstruction(cntxt->fdout,mb, 0, q, LIST_MAL_ALL);
#endif
					if( q->argc == r->argc ){
						clrFunction(q);
						getArg(q,q->retc) = getArg(r,0);
						q->argc = q->retc + 1;
					} else {
						getArg(q,q->retc) = getArg(r,0);
						for( k= q->retc +1 ; k < prefixlength; k++)
							delArgument(q, q->retc + 1);
						if( q->argc == 3)
							setFunctionId(q,projectionRef);
					}
#ifdef DEBUG_OPT_PROJECTIONPATH
					mnstr_printf(cntxt->fdout,"#projectionpath after :");
					printInstruction(cntxt->fdout,mb, 0, q, LIST_MAL_ALL);
#endif
				}
			}
			/* patch instruction p by deletion of common prefix */
			if( r->argc == p->argc ){
				clrFunction(p);
				getArg(p,p->retc) = getArg(r,0);
				p->argc = p->retc + 1;
			} else {
				getArg(p,p->retc) = getArg(r,0);
				for( k= p->retc +  1; k < prefixlength; k++)
					delArgument(p, p->retc + 1);
				if( p->argc == 3)
					setFunctionId(p,projectionRef);
			}

			OPTDEBUGprojectionpath 
				printInstruction(cntxt->fdout,mb, 0, p, LIST_MAL_ALL);
		}
		pushInstruction(mb,p);
	}
#ifdef DEBUG_OPT_PROJECTIONPATH
	chkProgram(cntxt->fdout, cntxt->nspace, mb);
	mnstr_printf(cntxt->fdout,"#projectionpath prefix actions %d\n",actions);
	if(actions) printFunction(cntxt->fdout,mb, 0, LIST_MAL_ALL);
#endif
	for(; i<slimit; i++)
		if(old[i])
			freeInstruction(old[i]);
	GDKfree(old);
	if( actions)
		actions += OPTdeadcodeImplementation(cntxt, mb, 0, 0);
	return actions;
}
#endif

int
OPTprojectionpathImplementation(Client cntxt, MalBlkPtr mb, MalStkPtr stk, InstrPtr p)
{
	int i,j,k, actions=0, maxprefixlength=0;
	int *pc =0;
	InstrPtr q,r;
	InstrPtr *old=0;
	int *varcnt= 0;		/* use count */
	int limit,slimit;
	char buf[256];
	lng usec = GDKusec();

	(void) cntxt;
	(void) stk;
	if ( mb->inlineProp)
		return 0;

#ifdef DEBUG_OPT_PROJECTIONPATH
	mnstr_printf(cntxt->fdout,"#projectionpath optimizer start \n");
	printFunction(cntxt->fdout,mb, 0, LIST_MAL_ALL);
#endif
	old= mb->stmt;
	limit= mb->stop;
	slimit= mb->ssize;
	if ( newMalBlkStmt(mb,mb->ssize + mb->stop) < 0)
		return 0;

	/* beware, new variables and instructions are introduced */
	pc= (int*) GDKzalloc(sizeof(int)* mb->vtop * 2); /* to find last assignment */
	varcnt= (int*) GDKzalloc(sizeof(int)* mb->vtop * 2); 
	if (pc == NULL || varcnt == NULL )
		goto wrapupall;

	/*
	 * Count the variable re-use  used as arguments first.
	 * A pass operation is not a real re-use
	 */
	for (i = 0; i<limit; i++){
		p= old[i];
		for(j=p->retc; j<p->argc; j++)
		if( ! (getModuleId(p) == languageRef && getFunctionId(p)== passRef))
			varcnt[getArg(p,j)]++;
	}

	/* assume a single pass over the plan, and only consider projection sequences 
	 * beware, we are only able to deal with projections without candidate lists. (argc=3)
	 * We also should not change the type of the outcome, i.e. leaving the last argument untouched.
 	 */
	for (i = 0; i<limit; i++){
		p= old[i];
		if( getModuleId(p)== algebraRef && getFunctionId(p) == projectionRef && p->argc == 3){
			/*
			 * Try to expand its argument list with what we have found so far.
			 */
			q = copyInstruction(p);
#ifdef DEBUG_OPT_PROJECTIONPATH
			mnstr_printf(cntxt->fdout,"#before ");
			printInstruction(cntxt->fdout,mb, 0, p, LIST_MAL_ALL);
#endif
			q->argc=p->retc;
			for(j=p->retc; j<p->argc; j++){
				if (pc[getArg(p,j)] )
					r = getInstrPtr(mb,pc[getArg(p,j)]);
				else 
					r = 0;
				if (r && varcnt[getArg(p,j)] > 1 )
					r = 0;
				
				/* inject the complete sub-path */
#ifdef DEBUG_OPT_PROJECTIONPATH
				mnstr_printf(cntxt->fdout,"#inject ");
				printInstruction(cntxt->fdout,mb, 0, r, LIST_MAL_ALL);
#endif
				if ( getFunctionId(p) == projectionRef){
					if( r &&  getModuleId(r)== algebraRef && ( getFunctionId(r)== projectionRef  || getFunctionId(r)== projectionpathRef) ){
						for(k= r->retc; k<r->argc; k++) 
							q = pushArgument(mb,q,getArg(r,k));
					} else 
						q = pushArgument(mb,q,getArg(p,j));
				}
			}
			if(q->argc<= p->argc){
				/* no change */
				freeInstruction(q);
				goto wrapup;
			}
			/*
			 * Final type check and hardwire the result type, because that  can not be inferred directly from the signature
			 * We already know that all heads are void. Only the last element may have a non-oid type.
			 */
			for(j=1; j<q->argc-1; j++)
				if( getBatType(getArgType(mb,q,j)) != TYPE_oid  && getBatType(getArgType(mb,q,j)) != TYPE_void ){
					/* don't use the candidate list */
					freeInstruction(q);
					goto wrapup;
				}

			/* fix the type */
			setVarUDFtype(mb, getArg(q,0));
			setVarType(mb, getArg(q,0), newBatType(getBatType(getArgType(mb,q,q->argc-1))));
			if ( getFunctionId(q) == projectionRef )
				setFunctionId(q,projectionpathRef);
			q->typechk = TYPE_UNKNOWN;
#ifdef DEBUG_OPT_PROJECTIONPATH
			mnstr_printf(cntxt->fdout,"#after ");
			printInstruction(cntxt->fdout,mb, 0, q, LIST_MAL_ALL);
#endif
			freeInstruction(p);
			p = q;
			/* keep track of the longest projection path */
			if ( p->argc  > maxprefixlength)
				maxprefixlength = p->argc;
			actions++;
		} 
	wrapup:
		pushInstruction(mb,p);
		for(j=0; j< p->retc; j++)
		if( getModuleId(p)== algebraRef && ( getFunctionId(p)== projectionRef  || getFunctionId(p)== projectionpathRef) ){
			pc[getArg(p,j)]= mb->stop-1;
#ifdef DEBUG_OPT_PROJECTIONPATH
			mnstr_printf(cntxt->fdout,"#keep ");
			printInstruction(cntxt->fdout,mb, 0, p, LIST_MAL_ALL);
#endif
		}
	}
#ifdef DEBUG_OPT_PROJECTIONPATH
		mnstr_printf(cntxt->fdout,"#projection path prefixlength %d\n",maxprefixlength);
#endif

	for(; i<slimit; i++)
		if(old[i])
			freeInstruction(old[i]);

	/* All complete projection paths have been constructed.
	 * There may be cases where there is a common prefix used multiple times.
	 * They are located and removed in a few scans over the plan
	 *
	 * The prefix path mostly consist of smaller columns,
	 * which make the benefit not large. In SF100 roughly 100 out of
	 * 4500 projection operations were removed.
	 * On medium scale databases it may save cpu cycles.
	 * Turning this feature into a compile time option.
	 */
#ifdef ELIMCOMMONPREFIX
	if( maxprefixlength > 3){
		 /* Before searching the prefix, we should remove all non-used instructions.  */
		actions += OPTdeadcodeImplementation(cntxt, mb, 0, 0);
		for( ; maxprefixlength > 2; maxprefixlength--)
			actions += OPTprojectionPrefix(cntxt, mb, maxprefixlength);
	}
#endif
#ifdef DEBUG_OPT_PROJECTIONPATH
	mnstr_printf(cntxt->fdout,"#projectionpath optimizer result \n");
	printFunction(cntxt->fdout,mb, 0, LIST_MAL_ALL);
#endif

    /* Defense line against incorrect plans */
    if( actions > 0){
        chkTypes(cntxt->fdout, cntxt->nspace, mb, FALSE);
        chkFlow(cntxt->fdout, mb);
        chkDeclarations(cntxt->fdout, mb);
    }
    /* keep all actions taken as a post block comment */
    snprintf(buf,256,"%-20s actions=%2d time=" LLFMT " usec","projectionpath",actions,GDKusec() - usec);
    newComment(mb,buf);
wrapupall:
	if (pc ) GDKfree(pc);
	if (varcnt ) GDKfree(varcnt);
	if(old) GDKfree(old);

	return actions;
}
