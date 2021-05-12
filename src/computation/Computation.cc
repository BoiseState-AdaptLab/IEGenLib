/*!
 * \file Computation.cc
 *
 * \brief Implementation of the Computation and Stmt classes, and supporting
 * classes.
 *
 * The Computation class is the SPF representation of a logical computation.
 * It contains a Stmt class for each statement, which in turn contains
 * information about that statement as mathematical objects.
 * Originally part of spf-ie.
 *
 * \date Started: 10/09/20
 *
 * \authors Anna Rift
 *
 * Copyright (c) 2020, University of Arizona <br>
 * Copyright (c) 2020, Boise State University <br>
 * All rights reserved. <br>
 * See ../../COPYING for details. <br>
 */

#include "Computation.h"
#include "util.h"

#include <code_gen/parser/parser.h>
#include <codegen.h>


#include <algorithm>
#include <set>
#include <sstream>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>
#include <map>

#include "set_relation/set_relation.h"

//! Base string for use in name prefixing
#define NAME_PREFIX_BASE "_iegen_"

namespace iegenlib {

/* Computation */

unsigned int Computation::numRenames = 0;

Computation::Computation() {}

Computation::~Computation() {
    clear();
}

Computation::Computation(const Computation& other) { *this = other; }

Computation& Computation::operator=(const Computation& other) {
    this->stmts = other.stmts;
    this->dataSpaces = other.dataSpaces;
    this->parameters = other.parameters;
    this->returnValues = other.returnValues;

    return *this;
}

bool Computation::operator==(const Computation& other) const {
    return (this->stmts == other.stmts &&
            this->dataSpaces == other.dataSpaces &&
            this->parameters == other.parameters &&
            this->returnValues == other.returnValues);
}

Computation* Computation::getUniquelyNamedClone() const {
    std::string namePrefix = NAME_PREFIX_BASE + std::to_string(numRenames++);
    Computation* prefixedCopy = new Computation();

    // prefix all data in the Computation and insert it to the new one
    for (auto& stmt : this->stmts) {
        prefixedCopy->addStmt(stmt->getUniquelyNamedClone(namePrefix));
    }
    for (auto& space : this->dataSpaces) {
        prefixedCopy->addDataSpace(namePrefix + space);
    }
    for (auto& param : this->parameters) {
        prefixedCopy->addParameter(namePrefix + param.first, param.second);
    }
    for (auto& retVal : this->returnValues) {
        // only prefix values that are data space names, avoid trying to prefix
        // literals
        prefixedCopy->addReturnValue(
            (retVal.second ? namePrefix : "") + retVal.first, retVal.second);
    }

    return prefixedCopy;
}

void Computation::resetNumRenames() {
    Computation::numRenames = 0;
}

void Computation::addStmt(Stmt* stmt) {
    stmts.push_back(stmt);
}

Stmt* Computation::getStmt(unsigned int index) { return stmts.at(index); }

unsigned int Computation::getNumStmts() const { return stmts.size(); }

void Computation::addDataSpace(std::string dataSpaceName) {
    dataSpaces.emplace(dataSpaceName);
}

std::unordered_set<std::string> Computation::getDataSpaces() const {
    return dataSpaces;
}

bool Computation::isDataSpace(std::string name) const {
    return dataSpaces.count(name) > 0;
}

void Computation::addParameter(std::string paramName, std::string paramType) {
    parameters.push_back({paramName, paramType});
    // parameters are automatically available as data spaces to the Computation
    addDataSpace(paramName);
}

std::string Computation::getParameterName(unsigned int index) const {
    return parameters.at(index).first;
}

std::string Computation::getParameterType(unsigned int index) const {
    return parameters.at(index).second;
}

unsigned int Computation::getNumParams() const {
    return parameters.size();
}

void Computation::addReturnValue(std::string name) {
    returnValues.push_back({name, this->isDataSpace(name)});
}

void Computation::addReturnValue(std::string name, bool isDataSpace) {
    returnValues.push_back({name, isDataSpace});
}

std::vector<std::string> Computation::getReturnValues() const {
    std::vector<std::string> names;
    for (const auto& retVal : returnValues) {
        names.push_back(retVal.first);
    }
    return names;
}

unsigned int Computation::getNumReturnValues() const {
    return returnValues.size();
}

void Computation::printInfo() const {
    std::ostringstream stmtsOutput;
    std::ostringstream iterSpacesOutput;
    std::ostringstream execSchedulesOutput;
    std::ostringstream dataReadsOutput;
    std::ostringstream dataWritesOutput;
    std::ostringstream dataSpacesOutput;

    int stmtNum = 0;
    for (const auto& it : stmts) {
        std::string stmtName = "S" + std::to_string(stmtNum);
        // stmt source code
        stmtsOutput << stmtName << ": " << (*it).getStmtSourceCode() << "\n";
        // iter spaces
        iterSpacesOutput << stmtName << ": "
                         << (*it).getIterationSpace()->prettyPrintString() << "\n";
        // exec schedules
        execSchedulesOutput << stmtName << ": "
                            << (*it).getExecutionSchedule()->prettyPrintString()
                            << "\n";
        // data reads
        unsigned int numReads = (*it).getNumReads();
        dataReadsOutput << stmtName << ":";
        if (numReads == 0) {
            dataReadsOutput << " none";
        } else {
            dataReadsOutput << "{\n";
            for (unsigned int i = 0; i < numReads; ++i) {
                dataReadsOutput << "    " << (*it).getReadDataSpace(i) << ": "
                                << (*it).getReadRelation(i)->prettyPrintString()
                                << "\n";
            }
            dataReadsOutput << "}";
        }
        dataReadsOutput << "\n";
        // data writes
        unsigned int numWrites = (*it).getNumWrites();
        dataWritesOutput << stmtName << ":";
        if (numWrites == 0) {
            dataWritesOutput << " none";
        } else {
            dataWritesOutput << "{\n";
            for (unsigned int i = 0; i < numWrites; ++i) {
                dataWritesOutput << "    " << (*it).getReadDataSpace(i) << ": "
                                 << (*it).getReadRelation(i)->prettyPrintString()
                                 << "\n";
            }
            dataWritesOutput << "}";
        }
        dataWritesOutput << "\n";
        stmtNum++;
    }

    // data spaces
    dataSpacesOutput << "{";
    for (const auto& it : dataSpaces) {
        if (it != *dataSpaces.begin()) {
            dataSpacesOutput << ", ";
        }
        dataSpacesOutput << it;
    }
    dataSpacesOutput << "}\n";

    std::cout << "Statements:\n" << stmtsOutput.str();
    std::cout << "\nIteration spaces:\n" << iterSpacesOutput.str();
    std::cout << "\nExecution schedules:\n" << execSchedulesOutput.str();
    std::cout << "\nData spaces: " << dataSpacesOutput.str();
    std::cout << "\nArray reads:\n" << dataReadsOutput.str();
    std::cout << "\nArray writes:\n" << dataWritesOutput.str();
    std::cout << "\n";
}

bool Computation::isComplete() const {
    std::unordered_set<std::string> dataSpacesActuallyAccessed;
    for (const auto& stmt : stmts) {
        // check completeness of each statement
        if (!stmt->isComplete()) {
            return false;
        }

        // collect all data space accesses
        for (unsigned int i = 0; i < stmt->getNumReads(); ++i) {
            dataSpacesActuallyAccessed.emplace(stmt->getReadDataSpace(i));
        }
        for (unsigned int i = 0; i < stmt->getNumWrites(); ++i) {
            dataSpacesActuallyAccessed.emplace(stmt->getWriteDataSpace(i));
        }
    }

    // check that list of data spaces matches those accessed in statements
    if (dataSpaces != dataSpacesActuallyAccessed) {
        return false;
    }

    return true;
}

void Computation::clear() {
    for (auto& stmt : stmts) {
        delete stmt;
    }
    stmts.clear();
    dataSpaces.clear();
}

AppendComputationResult Computation::appendComputation(
    const Computation* other, std::string surroundingIterDomainStr,
    std::string surroundingExecScheduleStr,
    const std::vector<std::string>& arguments) {
    Set* surroundingIterDomain = new Set(surroundingIterDomainStr);
    Relation* surroundingExecSchedule = new Relation(surroundingExecScheduleStr);

    // create a working copy of the appendee
    Computation* toAppend = other->getUniquelyNamedClone();
    const unsigned int numArgs = arguments.size();

    // gather append context's information
    const TupleDecl surroundingExecTuple =
        surroundingExecSchedule->getTupleDecl();
    const unsigned int surroundingExecInArity =
        surroundingExecSchedule->inArity();
    const unsigned int surroundingExecOutArity =
        surroundingExecSchedule->outArity();
    if (surroundingExecSchedule->getNumConjuncts() != 1) {
        throw assert_exception(
            "Surrounding execution schedule should have exactly 1 "
            "Conjunction.");
    }
    const TupleDecl surroundingIterTuple =
        surroundingIterDomain->getTupleDecl();
    const int surroundingIterArity = surroundingIterDomain->arity();
    const bool surroundingContextHasIterators =
        !surroundingIterTuple.elemIsConst(0);
    if (surroundingIterDomain->getNumConjuncts() != 1) {
        throw assert_exception(
            "Surrounding iteration domain should have exactly 1 Conjunction.");
    }

    // ensure that arguments match parameter list length
    if (numArgs != toAppend->getNumParams()) {
        throw assert_exception(
            "Incorrect number of parameters specified for appendComputation -- "
            "expected " +
            std::to_string(toAppend->getNumParams()) + ", got " +
            std::to_string(numArgs));
    }

    // Insert declarations+assignment of (would-have-been if not for inlining)
    // parameter values, at the beginning of the appendee.
    // Assignment of a parameter i will be like:
    // [type of i] [name of i] = [name of argument i from passed-in list];
    // Insertion is done by prepending statements one at a time in reverse
    // order.
    for (int i = ((signed int)numArgs) - 1; i >= 0; --i) {
        Stmt* paramDeclStmt = new Stmt();

        paramDeclStmt->setStmtSourceCode(toAppend->getParameterType(i) + " " +
                                         toAppend->getParameterName(i) + " = " +
                                         arguments[i] + ";");
        paramDeclStmt->setIterationSpace("{[0]}");
        paramDeclStmt->setExecutionSchedule("{[0]->[" + std::to_string(i) +
                                            "]}");

        // If passed-in argument is a data space, mark it as being read
        // (otherwise it is a literal)
        if (this->isDataSpace(arguments[i])) {
            paramDeclStmt->addRead(arguments[i], "{[0]->[0]}");
        }
        paramDeclStmt->addWrite(toAppend->getParameterName(i), "{[0]->[0]}");

        toAppend->stmts.insert(toAppend->stmts.begin(), paramDeclStmt);
    }

    // calculate indexes/offsets for execution tuple modifications
    // value to offset 0th position of appended schedule tuples by
    int offsetValue =
        surroundingExecTuple.elemConstVal(surroundingExecSchedule->arity() - 1);
    // Keep track of the latest execution schedule position used.
    int latestTupleValue = offsetValue;

    // construct and insert an adjusted version of each statement of appendee,
    // including parameter declaration statements
    unsigned int remainingParamDeclStmts = numArgs;
    bool processingOriginalStmts = false;
    for (unsigned int stmtNum = 0; stmtNum < toAppend->getNumStmts();
         ++stmtNum) {
        // once we've finished processing prepended parameter declaration
        // statements, increase the offset for remaining (original) statements
        // by the number of prepended statements
        if (!processingOriginalStmts) {
            if (remainingParamDeclStmts == 0) {
                offsetValue += numArgs;
                processingOriginalStmts = true;
            } else {
                remainingParamDeclStmts--;
            }
        }

        // statement to copy and modify from the appendee
        Stmt* appendeeStmt = toAppend->getStmt(stmtNum);
        // new statement built up from appendee which will be inserted
        Stmt* newStmt = new Stmt();

        /* Source code */
        newStmt->setStmtSourceCode(appendeeStmt->getStmtSourceCode());

        /* Iteration domain */
        // collect information about current iteration space
        Set* appendIterSpace = appendeeStmt->getIterationSpace();
        const int appendIterArity = appendIterSpace->arity();
        TupleDecl appendIterTuple = appendIterSpace->getTupleDecl();

        // construct new iteration space
        Set* newIterSpace;
        if (!surroundingContextHasIterators) {
            // if iteration domain for surrounding context is a singleton, leave
            // append iteration domain as-is
            newIterSpace = new Set(*appendIterSpace);
        } else if (appendIterTuple.elemIsConst(0)) {
            // if iteration domain to append is a singleton, adopt surrounding
            // iteration domain
            newIterSpace = new Set(*surroundingIterDomain);
        } else {
            // if neither iteration domain is trivial, combine the two

            // collect pieces for a new iteration space
            // construct tuple with desired iterators after appending
            TupleDecl newIterTuple =
                TupleDecl(surroundingIterArity + appendIterArity);
            for (unsigned int i = 0; i < surroundingIterArity; ++i) {
                newIterTuple.copyTupleElem(surroundingIterTuple, i, i);
            }
            for (unsigned int i = 0; i < appendIterArity; ++i) {
                newIterTuple.copyTupleElem(appendIterTuple, i,
                                           surroundingIterArity + i);
            }

            // add constraints from surroundings
            Conjunction* newIterSpaceConj = new Conjunction(newIterTuple);
            newIterSpaceConj->copyConstraintsFrom(
                *surroundingIterDomain->conjunctionBegin());

            // remap constraints from appendee iteration space to fit new tuple,
            // then copy them in
            Conjunction* shiftedAppendIterSpaceConj =
                new Conjunction(**appendIterSpace->conjunctionBegin());
            std::vector<int> shiftAppendeeIters;
            for (unsigned int i = 0; i < appendIterArity; ++i) {
                shiftAppendeeIters.push_back(surroundingIterArity + i);
            }
            shiftedAppendIterSpaceConj->remapTupleVars(shiftAppendeeIters);
            newIterSpaceConj->copyConstraintsFrom(shiftedAppendIterSpaceConj);
            delete shiftedAppendIterSpaceConj;

            // construct the new iteration space
            newIterSpace = new Set(newIterTuple);
            newIterSpace->addConjunction(newIterSpaceConj);
        }
        newStmt->setIterationSpace(newIterSpace->prettyPrintString());

        /* Execution schedule */
        // original execution schedule for statement to be appended
        Relation* appendExecSchedule = appendeeStmt->getExecutionSchedule();
        TupleDecl appendExecTuple = appendExecSchedule->getTupleDecl();
        int appendExecInArity = appendExecSchedule->inArity();
        int appendExecOutArity = appendExecSchedule->outArity();

        // construct new execution schedule tuple
        int newExecInArity = surroundingExecInArity + appendExecInArity;
        // Subtract space for '0' iterator placeholder, if present. Only counted
        // once because if neither one has real iterators, a '0' will be used.
        if (!surroundingContextHasIterators || appendExecTuple.elemIsConst(0)) {
            newExecInArity -= 1;
        }
        int newExecOutArity = surroundingExecSchedule->outArity() +
                              appendExecSchedule->outArity() - 1;
        TupleDecl newExecTuple = TupleDecl(newExecInArity + newExecOutArity);

        unsigned int currentTuplePos = 0;
        // insert iterators from surrounding context
        for (int i = 0; i < surroundingExecSchedule->inArity(); ++i) {
            // skip '0' iterator placeholder
            if (i == 0 && surroundingExecTuple.elemIsConst(0)) {
                continue;
            }
            newExecTuple.copyTupleElem(surroundingExecTuple, i,
                                       currentTuplePos++);
        }
        // insert iterators from appended statement
        for (int i = 0; i < appendExecInArity; ++i) {
            // skip '0' iterator placeholder
            if (i == 0 && appendExecTuple.elemIsConst(0)) {
                continue;
            }
            newExecTuple.copyTupleElem(appendExecTuple, i, currentTuplePos++);
        }
        // if neither surroundings nor appended stmt have iterators, insert the
        // placeholder '0' iterator
        if (currentTuplePos == 0) {
            newExecTuple.setTupleElem(currentTuplePos++, 0);
        }
        // insert surrounding schedule tuple elements except the last one, which
        // must be combined with first append tuple value
        for (int i = surroundingExecSchedule->inArity();
             i < surroundingExecSchedule->arity() - 1; ++i) {
            newExecTuple.copyTupleElem(surroundingExecTuple, i,
                                       currentTuplePos++);
        }
        // offset and insert first append tuple value
        latestTupleValue =
            appendExecTuple.elemConstVal(appendExecInArity) + offsetValue;
        newExecTuple.setTupleElem(currentTuplePos++, latestTupleValue);
        // insert remaining append tuple values
        for (int i = appendExecInArity + 1; i < appendExecTuple.size(); ++i) {
            newExecTuple.copyTupleElem(appendExecTuple, i, currentTuplePos++);
        }

        // create new execution schedule Relation using (only) the new tuple
        newStmt->setExecutionSchedule(
            "{" + newExecTuple.toString(true, newExecInArity) + "}");

        /* Data reads */
        for (unsigned int i = 0; i < appendeeStmt->getNumReads(); ++i) {
            Relation* appendeeReadRel = appendeeStmt->getReadRelation(i);
            Relation* newReadRel;
            // only need to adjust if there are iterators in surrounding context
            if (surroundingContextHasIterators) {
                const int oldAppendInArity = appendeeReadRel->inArity();
                const int oldAppendOutArity = appendeeReadRel->outArity();
                // shift appendee relation tuple to make room for new iterators
                int shiftDistance = surroundingIterArity;
                if (appendeeReadRel->getTupleDecl().elemIsConst(0)) {
                    // no need to shift out a 0, it can be written over
                    shiftDistance -= 1;
                }
                std::vector<int> shiftReadRel;
                for (unsigned int pos = 0; pos < appendeeReadRel->arity();
                     ++pos) {
                    shiftReadRel.push_back(shiftDistance + pos);
                }
                appendeeReadRel->remapTupleVars(shiftReadRel);
                appendeeReadRel->SetinArity(shiftDistance + oldAppendInArity);

                // insert new iterators into tuple
                TupleDecl shiftedAppendeeReadTuple =
                    appendeeReadRel->getTupleDecl();
                for (unsigned int pos = 0; pos < surroundingIterArity; ++pos) {
                    shiftedAppendeeReadTuple.copyTupleElem(surroundingIterTuple,
                                                           pos, pos);
                }

                // construct new read relation
                newReadRel = new Relation(oldAppendInArity + shiftDistance, oldAppendOutArity);
                Conjunction* newReadRelConj = new Conjunction(shiftedAppendeeReadTuple);
                newReadRelConj->setInArity(newReadRel->inArity());
                newReadRelConj->copyConstraintsFrom(*appendeeReadRel->conjunctionBegin());
                newReadRel->addConjunction(newReadRelConj);
            } else {
                newReadRel = new Relation(*appendeeReadRel);
            }

            // add the new read using the new Relation
            newStmt->addRead(appendeeStmt->getReadDataSpace(i),
                             newReadRel->prettyPrintString());
        }


        /* Data writes */
        for (unsigned int i = 0; i < appendeeStmt->getNumWrites(); ++i) {
            Relation* appendeeWriteRel = appendeeStmt->getWriteRelation(i);
            Relation* newWriteRel;
            // only need to adjust if there are iterators in surrounding context
            if (surroundingContextHasIterators) {
                const int oldAppendInArity = appendeeWriteRel->inArity();
                const int oldAppendOutArity = appendeeWriteRel->outArity();
                // shift appendee relation tuple to make room for new iterators
                int shiftDistance = surroundingIterArity;
                if (appendeeWriteRel->getTupleDecl().elemIsConst(0)) {
                    // no need to shift out a 0, it can be written over
                    shiftDistance -= 1;
                }
                std::vector<int> shiftWriteRel;
                for (unsigned int pos = 0; pos < appendeeWriteRel->arity();
                     ++pos) {
                    shiftWriteRel.push_back(shiftDistance + pos);
                }
                appendeeWriteRel->remapTupleVars(shiftWriteRel);
                appendeeWriteRel->SetinArity(shiftDistance + oldAppendInArity);

                // insert new iterators into tuple
                TupleDecl shiftedAppendeeWriteTuple =
                    appendeeWriteRel->getTupleDecl();
                for (unsigned int pos = 0; pos < surroundingIterArity; ++pos) {
                    shiftedAppendeeWriteTuple.copyTupleElem(surroundingIterTuple,
                                                           pos, pos);
                }

                // construct new write relation
                newWriteRel = new Relation(oldAppendInArity + shiftDistance, oldAppendOutArity);
                Conjunction* newWriteRelConj = new Conjunction(shiftedAppendeeWriteTuple);
                newWriteRelConj->setInArity(newWriteRel->inArity());
                newWriteRelConj->copyConstraintsFrom(*appendeeWriteRel->conjunctionBegin());
                newWriteRel->addConjunction(newWriteRelConj);
            } else {
                newWriteRel = new Relation(*appendeeWriteRel);
            }

            // add the new write using the new Relation
            newStmt->addWrite(appendeeStmt->getWriteDataSpace(i),
                             newWriteRel->prettyPrintString());
        }

        // add the adapted statement into this Computation
        this->addStmt(newStmt);
    }

    // add (already name prefixed) data spaces from appendee to appender
    for (const auto& dataSpace : toAppend->getDataSpaces()) {
        this->addDataSpace(dataSpace);
    }

    // collect append result information to return
    AppendComputationResult result;
    result.tuplePosition = latestTupleValue;
    result.returnValues = toAppend->getReturnValues();

    delete surroundingIterDomain;
    delete surroundingExecSchedule;
    delete toAppend;

    return result;
}

//! Function returns a dot string representing nesting
//  and loop carrie dependency. Internally it uses
//  a lite version of polyhedral scanning to generate
//  subgraphs in the dot file.
//
std::string Computation::toDotString(){

    
    // First apply all transformations to a clone of computation.
    std::vector<Set*> newIS;
    std::ostringstream ss;
    //TODO: Deal with disjunction of cunjunctions later.
    for(Stmt* st : stmts){
        Set* appSet = st->getExecutionSchedule()->Apply(
			st->getIterationSpace());
	newIS.push_back(appSet);
    }
    int maxLevel = newIS[0]->arity();
    std::vector<std::vector<Set*> > projectedIS(maxLevel);
    
    for(int i = 0 ; i < maxLevel; i++ ){
        projectedIS[i] = std::vector<Set*>(newIS.size());
    }

    for(int i = 0; i < stmts.size(); i++){
        if (maxLevel > 0)
            projectedIS[maxLevel-1][i] = newIS[i];
	//Perform projections for each column
	for (int j = maxLevel -1; j >= 1 ; j --){
	    projectedIS[j -1][i] = projectedIS[j][i]->projectOut(j);   
	}
    }
    omega::BoolSet<>active;
    active.set_all();
    Set * restriction  = new Set(maxLevel);
    ss << "digraph dataFlowGraph_1{ \n";
    
    generateToDotClusters(projectedIS,maxLevel,active,restriction,
		    1,ss);
    
    // Add Data Dependence edges , Extracted directly from Shivani's
    // codebase
     
    std::vector<string>
        data_spaces;  // Maintains the list of dataspaces already created
    
    // Adding the participating dataspaces for each statement and mapping out
    // the read and write access.
    for (int i = 0; i < getNumStmts(); i++) {
        // Iterates over the read-DataSpaces
        for (int data_read_index = 0;
             data_read_index < getStmt(i)->getNumReads(); data_read_index++) {
            string readDataSpace =
                getStmt(i)->getReadDataSpace(data_read_index);
            // Check to make sure the data space is not created if it already
            // exists
            if (!(std::count(data_spaces.begin(), data_spaces.end(),
                             readDataSpace))) {
                // Creates data space
                ss
                    << "\t\t"
                    << "subgraph cluster_dataspace" << readDataSpace << "{ \n"
                    << "\t\t\t"
                    << "style = filled; \n"
                    << "\t\t\t"
                    << "color = lightgrey; \n"
                    << "\t\t\t"
                    << "label=\" \"; \n"
                    << "\t\t\t" << readDataSpace << "[label=\"" << readDataSpace
                    << "[] \"] [shape=box][style=filled][color=lightgrey];\n"
                    << "\t\t\t"
                    << "}\n";

                data_spaces.push_back(readDataSpace);
            }

            size_t start_pos = getStmt(i)
                                   ->getReadRelation(data_read_index)
                                   ->getString()
                                   .rfind("[");
            size_t end_pos = getStmt(i)
                                 ->getReadRelation(data_read_index)
                                 ->getString()
                                 .rfind("]");

            ss << "\t\t" << readDataSpace << "->"
                    << "S" << i << "[label=\"["
                    << getStmt(i)
                           ->getReadRelation(data_read_index)
                           ->getString()
                           .substr(start_pos + 1, end_pos - start_pos - 1)
                    << "]\"]"
                    << "\n";
        }

        // Iterates over the read-DataSpaces
        for (int data_write_index = 0;
             data_write_index < getStmt(i)->getNumWrites();
             data_write_index++) {
            string writeDataSpace =
                getStmt(i)->getWriteDataSpace(data_write_index);
            // Check to make sure the data space is not created if it already
            // exists
            if (!(std::count(data_spaces.begin(), data_spaces.end(),
                             writeDataSpace))) {
                ss
                    << "\t\t"
                    << "subgraph cluster_dataspace" << writeDataSpace << "{ \n"
                    << "\t\t\t"
                    << "style = filled; \n"
                    << "\t\t\t"
                    << "color = lightgrey; \n"
                    << "\t\t\t"
                    << "label= \" \"; \n"
                    << "\t\t\t" << writeDataSpace << "[label=\""
                    << writeDataSpace
                    << "[] \"] [shape=box][style=filled][color=lightgrey];\n"
                    << "\t\t\t"
                    << "}\n";

                data_spaces.push_back(writeDataSpace);
            }

            size_t start_pos = getStmt(i)
                                   ->getWriteRelation(data_write_index)
                                   ->getString()
                                   .find("[");
            size_t end_pos = getStmt(i)
                                 ->getWriteRelation(data_write_index)
                                 ->getString()
                                 .find("]");

            ss << "\t\t"
                    << "S" << i << "->" << writeDataSpace << "[label=\"["
                    << getStmt(i)
                           ->getWriteRelation(data_write_index)
                           ->getString()
                           .substr(start_pos + 1, end_pos - start_pos - 1)
                    << "]\"]"
                    << "\n";
        }
    }

    ss << "}\n";
    return ss.str();
}

//! Helper function that checks if a condition results in active
//  sets.
omega::BoolSet<> Computation::getActive(omega::BoolSet<>&active,Set* cond,
                           std::vector<Set*> Rs ){
    omega::BoolSet<> result(active.size());
    for (omega::BoolSet<>::const_iterator i = active.begin();
		i != active.end(); i++){
        Set* splitSet = Rs[*i]->Intersect(cond);
	auto conj = *(splitSet->conjunctionBegin());
        if (conj->satisfiable()){
	    result.set(*i);  
	}
	delete splitSet;
         
    }
    return result;    
}

//! Recursively generate nodes in 
//  to dot.
//  param projectedIS presaved projections and levels 
//  param maxLevel    max level in set of disjunct poly
//                    hedrons.
void Computation::generateToDotClusters(std::vector<std::vector<Set*> >&projectedIS,
		    int maxLevel,omega::BoolSet<>& active,Set* restriction,
		    int currentLevel,std::ostringstream& ss){
    if (currentLevel > maxLevel){
        for (omega::BoolSet<>::const_iterator i = active.begin();
		i != active.end(); i++){
            // Print statement
	    ss << "S" << (*i) << "[label= \" " 
	       << stmts[(*i)]->getStmtSourceCode()
               << "\"][shape=Mrecord][style=filled][color=lightgrey] ; \n"
               << "\t\t";
        
	}  
	return;
    }
    int numActiveStmts = active.num_elem();

    if (numActiveStmts == 0)
        return;
    else if (numActiveStmts == 1){
        
        ss << '\t' << "subgraph cluster_D" << currentLevel << " { \n"
            << "\t\t"
            << "style = bold; \n"
            << "\t\t"
            << "color = grey; \n"
            << "\t\t"
            << ""
            << "\t\t"
            << "label = \" Domain: "
            << restriction->prettyPrintString()
            << "\"; \n";
    	generateToDotClusters(projectedIS,maxLevel,active,restriction,
	    currentLevel+1,ss);
        ss << "}";
	return;
    }
    // Look for a split
    std::vector<Set*> Rs(active.size());
    for (omega::BoolSet<>::const_iterator i = active.begin();
		i != active.end(); i++){
        Rs[*i] = projectedIS[currentLevel-1][*i]->Intersect(restriction); 
    }    
    
    for (omega::BoolSet<>::const_iterator i = active.begin();
		i != active.end(); i++){
        // for reach constraint in Rs
	
	// conjunction must be one at this point
	
        if (Rs[*i]->getNumConjuncts() != 1) {
            throw assert_exception(
                "Must have exactly one conjunction for Graph Visualization");
        }
        
	auto conj = *(Rs[*i]->conjunctionBegin());
        for(auto equalityConstraint: conj->equalities()){
           if (equalityConstraint->dependsOn(TupleVarTerm(currentLevel-1))){
               
               Set * cond = new Set(Rs[*i]->arity());
               Conjunction* condConj = new Conjunction(Rs[*i]->arity());
	       condConj->addEquality(equalityConstraint);
               cond->addConjunction(condConj);
               
               Set * condComp = islSetComplement(cond);
	      
               omega::BoolSet<>active1 = getActive(active,cond,Rs);
	       omega::BoolSet<>active2 = getActive(active,condComp,Rs);
               // Split if constraint splits the range 
	       // of the current level
	       if(!active1.empty() && !active2.empty() && 
	           /*active1 n active2 = 0*/
	           (active1 & active2).empty()){
	           // Add the condition to the restrictions
		   Set* restriction1 = restriction->Intersect(cond);
		   Set* restriction2 = restriction->Intersect(condComp);
		   generateToDotClusters(projectedIS,maxLevel,active1,
		       restriction1,currentLevel,ss);
                   
		   generateToDotClusters(projectedIS,maxLevel,active2,
		       restriction2,currentLevel,ss);
		   delete restriction1;
		   delete restriction2;
	       }
               delete cond;
               delete condComp;	      
	   } 
	}
        
	for(auto constraint: conj->inequalities()){
           if (constraint->dependsOn(TupleVarTerm(currentLevel-1))){
               Set * cond = new Set(Rs[*i]->arity());
	       Conjunction* condConj = new Conjunction(Rs[*i]->arity());
	       condConj->addInequality(constraint);
               cond->addConjunction(condConj);


               Set * condComp = islSetComplement(cond);
	      
               omega::BoolSet<>active1 = getActive(active,cond,Rs);
	       omega::BoolSet<>active2 = getActive(active,condComp,Rs);
               // Split if constraint splits the range 
	       // of the current level
	       if(!active1.empty() && !active2.empty() && 
	           /*active1 n active2 = 0*/
	           (active1 & active2).empty()){
	           // Add the condition to the restrictions
		   Set* restriction1 = restriction->Intersect(cond);
		   Set* restriction2 = restriction->Intersect(condComp);
		   generateToDotClusters(projectedIS,maxLevel,active1,
		       restriction1,currentLevel,ss);
		   generateToDotClusters(projectedIS,maxLevel,active2,
		       restriction2,currentLevel,ss);
		   delete restriction1;
		   delete restriction2;
	       }
               delete cond;
               delete condComp;	      
	   } 
	}	

    }
    // TODO: only generate subcluster if there exist a non
    //       constant constraint involvolving the current level    
    // Domain subgraph
    ss << '\t' << "subgraph cluster_D" << currentLevel << " { \n"
        << "\t\t"
        << "style = bold; \n"
        << "\t\t"
        << "color = grey; \n"
        << "\t\t"
        << ""
        << "\t\t"
        << "label = \" Domain: "
        << restriction->prettyPrintString()
        << "\"; \n";
    generateToDotClusters(projectedIS,maxLevel,active,restriction,
        currentLevel+1,ss);
    ss << "}";
    return;

}
//! param  activeStmt is assumed to be sorted lexicographically
std::vector<std::vector<Set*> > Computation::
   split (int level, std::vector<Set*> activeStmt){
   std::map<std::string,std::vector<Set*> > grouping;
   
   for(Set* s : activeStmt){
      if(s->getTupleDecl().elemIsConst(level)){
          grouping[std::to_string(
			  s->getTupleDecl().elemConstVal(level))].push_back(s); 
	     	  
      }else {
	  // This will be expanded further to use constraints;
          grouping["t"].push_back(s);
      }  
   }
   std::vector<std::vector <Set*> > res;
   for( auto k : grouping){
       //Next iteration of the algorithm will be
       //focused on this section. 
       res.push_back(k.second);
   }
   return res;
}

void Computation::toDot(std::fstream& dotFile, string fileName) {
    std::vector<string>
        data_spaces;  // Maintains the list of dataspaces already created
    std::vector<string> loop_variable;  // Maintains list of loop variables
    std::cout << "Reached toDot()"
              << "\n";
    dotFile.open(fileName,
                 std::ios::out);  // Open the file to write the dot code

    // Start of the dotFile
    dotFile << "digraph dataFlowGraph_1{ \n";

    // Adding nodes for each statement
    for (int i = 0; i < getNumStmts(); i++) {
        dotFile << '\t' << "subgraph cluster_S" << i << " { \n"
                << "\t\t"
                << "style = bold; \n"
                << "\t\t"
                << "color = grey; \n"
                << "\t\t"
                << ""
                << "\t\t"
                << "label = \" Domain: "
                << getStmt(i)->getIterationSpace()->prettyPrintString()
                << "\"; \n"
                << "\t\t"
                << "S" << i << "[label= \" " << getStmt(i)->getStmtSourceCode()
                << "\"][shape=Mrecord][style=filled][color=lightgrey] ; \n"
                << "\t\t"
                << "}";
    }

    // Adding the participating dataspaces for each statement and mapping out
    // the read and write access.
    for (int i = 0; i < getNumStmts(); i++) {
        // Iterates over the read-DataSpaces
        for (int data_read_index = 0;
             data_read_index < getStmt(i)->getNumReads(); data_read_index++) {
            string readDataSpace =
                getStmt(i)->getReadDataSpace(data_read_index);
            // Check to make sure the data space is not created if it already
            // exists
            if (!(std::count(data_spaces.begin(), data_spaces.end(),
                             readDataSpace))) {
                // Creates data space
                dotFile
                    << "\t\t"
                    << "subgraph cluster_dataspace" << readDataSpace << "{ \n"
                    << "\t\t\t"
                    << "style = filled; \n"
                    << "\t\t\t"
                    << "color = lightgrey; \n"
                    << "\t\t\t"
                    << "label=\" \"; \n"
                    << "\t\t\t" << readDataSpace << "[label=\"" << readDataSpace
                    << "[] \"] [shape=box][style=filled][color=lightgrey];\n"
                    << "\t\t\t"
                    << "}\n";

                data_spaces.push_back(readDataSpace);
            }

            size_t start_pos = getStmt(i)
                                   ->getReadRelation(data_read_index)
                                   ->getString()
                                   .rfind("[");
            size_t end_pos = getStmt(i)
                                 ->getReadRelation(data_read_index)
                                 ->getString()
                                 .rfind("]");

            dotFile << "\t\t" << readDataSpace << "->"
                    << "S" << i << "[label=\"["
                    << getStmt(i)
                           ->getReadRelation(data_read_index)
                           ->getString()
                           .substr(start_pos + 1, end_pos - start_pos - 1)
                    << "]\"]"
                    << "\n";
        }

        // Iterates over the read-DataSpaces
        for (int data_write_index = 0;
             data_write_index < getStmt(i)->getNumWrites();
             data_write_index++) {
            string writeDataSpace =
                getStmt(i)->getWriteDataSpace(data_write_index);
            // Check to make sure the data space is not created if it already
            // exists
            if (!(std::count(data_spaces.begin(), data_spaces.end(),
                             writeDataSpace))) {
                dotFile
                    << "\t\t"
                    << "subgraph cluster_dataspace" << writeDataSpace << "{ \n"
                    << "\t\t\t"
                    << "style = filled; \n"
                    << "\t\t\t"
                    << "color = lightgrey; \n"
                    << "\t\t\t"
                    << "label= \" \"; \n"
                    << "\t\t\t" << writeDataSpace << "[label=\""
                    << writeDataSpace
                    << "[] \"] [shape=box][style=filled][color=lightgrey];\n"
                    << "\t\t\t"
                    << "}\n";

                data_spaces.push_back(writeDataSpace);
            }

            size_t start_pos = getStmt(i)
                                   ->getWriteRelation(data_write_index)
                                   ->getString()
                                   .find("[");
            size_t end_pos = getStmt(i)
                                 ->getWriteRelation(data_write_index)
                                 ->getString()
                                 .find("]");

            dotFile << "\t\t"
                    << "S" << i << "->" << writeDataSpace << "[label=\"["
                    << getStmt(i)
                           ->getWriteRelation(data_write_index)
                           ->getString()
                           .substr(start_pos + 1, end_pos - start_pos - 1)
                    << "]\"]"
                    << "\n";
        }
    }

    dotFile << "}\n";
    dotFile.close();
}

std::string Computation::codeGen(Set* knownConstraints) {
    std::ostringstream generatedCode;

    // convert sets/relations to Omega format for use in codegen, and
    // collect statement macro definitions
    VisitorChangeUFsForOmega* vOmegaReplacer = new VisitorChangeUFsForOmega();
    std::vector<omega::Relation> transforms;
    std::vector<omega::Relation> iterSpaces;
    std::ostringstream stmtMacroUndefs;
    std::ostringstream stmtMacroDefs;
    int stmtCount = 0;
    for (const auto& stmt : stmts) {


	// new Codegen would require an application
	// be performed first before the set is sent
	// to omega. This is a temporary solution to
	// circumvent Omega's schedulling bug.
        Set * iterSpace = stmt->getExecutionSchedule()->
		Apply(stmt->getIterationSpace());
	iterSpace->acceptVisitor(vOmegaReplacer);
	std::string tupleString =
            iterSpace->getTupleDecl().toString();
        // Stmt Macro:
        stmtMacroUndefs << "#undef s" << stmtCount << "\n";
        stmtMacroDefs << "#define s" << stmtCount << "(" << tupleString
                      << ")   "
                      << iegenlib::replaceInString(stmt->getStmtSourceCode(), "$",
                                               "")
                      << " \n";
        stmtCount++;

        std::string omegaIterString =
            iterSpace->toOmegaString(vOmegaReplacer->getUFCallDecls());
        omega::Relation* omegaIterSpace =
            omega::parser::ParseRelation(omegaIterString);

        iterSpaces.push_back(omega::copy(*omegaIterSpace));
	// Use identity transformation instead.
        transforms.push_back(omega::Identity(iterSpace->arity()));
        delete omegaIterSpace;
	delete iterSpace;
    }

    // define necessary macros collected from statements
    std::ostringstream UFMacroUndefs;
    std::ostringstream UFMacroDefs;
    for (const auto& macro : *vOmegaReplacer->getUFMacros()) {
        UFMacroUndefs << "#undef " << macro.first << "\n";
        UFMacroDefs << "#define " << macro.first << " " << macro.second << "\n";
    }
    generatedCode << stmtMacroUndefs.str() << stmtMacroDefs.str() << "\n";
    generatedCode << UFMacroUndefs.str() << UFMacroDefs.str() << "\n";

    // initialize tuple variables
    std::ostringstream TupleAssignments;
    for(const auto& tupleA: vOmegaReplacer->getTupleAssignments()){
        TupleAssignments << "t" << (tupleA.first + 1) << " = "
		<< tupleA.second << "; \n";
    }
    generatedCode << TupleAssignments.str() << "\n";


    // convert set of known constraints to Omega format
    Set* modifiedKnown;
    if (knownConstraints) {
        modifiedKnown = new Set(*knownConstraints);
    } else {
        modifiedKnown = new Set("{}");
    }
    modifiedKnown->acceptVisitor(vOmegaReplacer);
    std::string omegaKnownString =
        modifiedKnown->toOmegaString(vOmegaReplacer->getUFCallDecls());
    delete modifiedKnown;
    omega::Relation* omegaKnown =
        omega::parser::ParseRelation(omegaKnownString);

    delete vOmegaReplacer;
    // do actual Omega CodeGen
    try {
        omega::CodeGen cg(transforms, iterSpaces, omega::copy(*omegaKnown));
        omega::CG_result* cgr = cg.buildAST();
        if (cgr) {
            generatedCode << cgr->printString() << "\n";
            delete cgr;
        } else {
            generatedCode << "/* empty */\n";
        }
    } catch (const std::exception& e) {
        std::cout << e.what() << std::endl;
    }
    delete omegaKnown;

    // undefine macros, which are now extraneous
    generatedCode << stmtMacroUndefs.str() << UFMacroUndefs.str();

    return generatedCode.str();
}


std::string Computation::toOmegaString() {
    std::ostringstream omegaString;

    // convert sets/relations to Omega format for use in codegen, and
    // collect statement macro definitions
    VisitorChangeUFsForOmega* vOmegaReplacer = new VisitorChangeUFsForOmega();
    int stmtCount = 0;
    for (const auto& stmt : stmts) {
        omegaString << "s" << stmtCount << "\n";
        omegaString << stmt->getStmtSourceCode() << "\n";
        stmtCount++;

	// new Codegen would require an application
	// be performed first before the set is sent
	// to omega. This is a temporary solution to
	// circumvent Omega's schedulling bug.
        Set * iterSpace = stmt->getExecutionSchedule()->
		Apply(stmt->getIterationSpace());
	iterSpace->acceptVisitor(vOmegaReplacer);


        std::string omegaIterString =
            iterSpace->toOmegaString(vOmegaReplacer->getUFCallDecls());
        omegaString << "Domain\n";
	omegaString << omegaIterString << "\n";

	delete iterSpace;
    }

    delete vOmegaReplacer;
    return omegaString.str();
}


/* Stmt */

Stmt::~Stmt() {
    iterationSpace.reset();
    executionSchedule.reset();
    for (auto& read : dataReads) {
        read.second.reset();
    }
    dataReads.clear();
    for (auto& write : dataWrites) {
        write.second.reset();
    }
    dataWrites.clear();
}

Stmt::Stmt(std::string stmtSourceCode, std::string iterationSpaceStr,
           std::string executionScheduleStr,
           std::vector<std::pair<std::string, std::string>> dataReadsStrs,
           std::vector<std::pair<std::string, std::string>> dataWritesStrs) {
    setStmtSourceCode(stmtSourceCode);
    setIterationSpace(iterationSpaceStr);
    setExecutionSchedule(executionScheduleStr);
    for (const auto& readInfo : dataReadsStrs) {
        dataReads.push_back(
            {readInfo.first,
             std::unique_ptr<Relation>(new Relation(readInfo.second))});
    }
    for (const auto& writeInfo : dataWritesStrs) {
        dataWrites.push_back(
            {writeInfo.first,
             std::unique_ptr<Relation>(new Relation(writeInfo.second))});
    }
};

Stmt::Stmt(const Stmt& other) { *this = other; }

Stmt& Stmt::operator=(const Stmt& other) {
    this->stmtSourceCode = other.stmtSourceCode;
    this->iterationSpace = std::unique_ptr<Set>(new Set(*other.iterationSpace));
    this->executionSchedule =
        std::unique_ptr<Relation>(new Relation(*other.executionSchedule));
    for (const auto& readInfo : other.dataReads) {
        this->dataReads.push_back(
            {readInfo.first,
             std::unique_ptr<Relation>(new Relation(*readInfo.second))});
    }
    for (const auto& writeInfo : other.dataWrites) {
        this->dataWrites.push_back(
            {writeInfo.first,
             std::unique_ptr<Relation>(new Relation(*writeInfo.second))});
    }
    return *this;
}

bool Stmt::operator==(const Stmt& other) const {
    // compare source code, iter space and exec schedule
    if (!(this->stmtSourceCode == other.stmtSourceCode &&
          *this->iterationSpace == *other.iterationSpace &&
          *this->executionSchedule == *other.executionSchedule)) {
        return false;
    }

    // compare data accesses, first by number then contents
    if (this->dataReads.size() != other.dataReads.size() ||
        this->dataWrites.size() != other.dataWrites.size()) {
        return false;
    }
    for (auto i = 0; i < this->dataReads.size(); ++i) {
        if (!(this->dataReads[i].first == other.dataReads[i].first &&
              *this->dataReads[i].second == *other.dataReads[i].second)) {
            return false;
        }
    }
    for (auto i = 0; i < this->dataWrites.size(); ++i) {
        if (!(this->dataWrites[i].first == other.dataWrites[i].first &&
              *this->dataWrites[i].second == *other.dataWrites[i].second)) {
            return false;
        }
    }

    return true;
}

Stmt* Stmt::getUniquelyNamedClone(std::string prefix) const {
    Stmt* prefixedCopy = new Stmt(*this);

    // modify reads and writes, keeping track of original names for further replacing in other fields
    std::unordered_set<std::string> dataSpaceNames;
    for (auto& read : prefixedCopy->dataReads) {
        dataSpaceNames.emplace(read.first);
        read.first = prefix + read.first;
    }
    for (auto& write : prefixedCopy->dataWrites) {
        dataSpaceNames.emplace(write.first);
        write.first = prefix + write.first;
    }

    // replace data space names in statement source code, iteration space, execution schedule
    std::string srcCode = prefixedCopy->stmtSourceCode;
    std::string iterSpaceStr = prefixedCopy->iterationSpace->prettyPrintString();
    std::string execScheduleStr = prefixedCopy->executionSchedule->prettyPrintString();
    for (const string& originalName : dataSpaceNames) {
        srcCode = replaceInString(srcCode, originalName, prefix + originalName);
        iterSpaceStr = replaceInString(iterSpaceStr, originalName, prefix + originalName);
        execScheduleStr = replaceInString(execScheduleStr, originalName, prefix + originalName);
    }
    // use modified strings to construct new values
    prefixedCopy->setStmtSourceCode(srcCode);
    prefixedCopy->setIterationSpace(iterSpaceStr);
    prefixedCopy->setExecutionSchedule(execScheduleStr);

    return prefixedCopy;
}

bool Stmt::isComplete() const {
    return !stmtSourceCode.empty() && iterationSpace && executionSchedule;
}

std::string Stmt::getStmtSourceCode() const { return stmtSourceCode; }

void Stmt::setStmtSourceCode(std::string newStmtSourceCode) {
    this->stmtSourceCode = newStmtSourceCode;
}

Set* Stmt::getIterationSpace() const { return iterationSpace.get(); }

void Stmt::setIterationSpace(std::string newIterationSpaceStr) {
    this->iterationSpace = std::unique_ptr<Set>(new Set(newIterationSpaceStr));
}

Relation* Stmt::getExecutionSchedule() const { return executionSchedule.get(); }

void Stmt::setExecutionSchedule(std::string newExecutionScheduleStr) {
    this->executionSchedule =
        std::unique_ptr<Relation>(new Relation(newExecutionScheduleStr));
}

void Stmt::addRead(std::string dataSpace, std::string relationStr) {
    dataReads.push_back(
        {dataSpace, std::unique_ptr<Relation>(new Relation(relationStr))});
}

unsigned int Stmt::getNumReads() const { return dataReads.size(); }

std::string Stmt::getReadDataSpace(unsigned int index) const {
    return dataReads.at(index).first;
}

Relation* Stmt::getReadRelation(unsigned int index) const {
    return dataReads.at(index).second.get();
}

void Stmt::addWrite(std::string dataSpace, std::string relationStr) {
    dataWrites.push_back(
        {dataSpace, std::unique_ptr<Relation>(new Relation(relationStr))});
}

unsigned int Stmt::getNumWrites() const { return dataWrites.size(); }

std::string Stmt::getWriteDataSpace(unsigned int index) const {
    return dataWrites.at(index).first;
}

Relation* Stmt::getWriteRelation(unsigned int index) const {
    return dataWrites.at(index).second.get();
}

/* VisitorChangeUFsForOmega */

VisitorChangeUFsForOmega::VisitorChangeUFsForOmega() { reset(); }

VisitorChangeUFsForOmega::~VisitorChangeUFsForOmega() { reset(); }

void VisitorChangeUFsForOmega::reset() {
    ufCallDecls.clear();
    macros.clear();
    knownUFs.clear();
    nextFuncReplacementNumber = 0;
    currentTupleDecl = NULL;
}

void VisitorChangeUFsForOmega::prepareForNext() {
    ufCallDecls.clear();
    currentTupleDecl = NULL;
    // known UFs are preserved for reference in next
    // macros are accumulated over Visitor lifetime
}

std::map<std::string, std::string>* VisitorChangeUFsForOmega::getUFMacros() {
    return &macros;
}

std::set<std::string> VisitorChangeUFsForOmega::getUFCallDecls() {
    return ufCallDecls;
}


std::map<int,std::string>& VisitorChangeUFsForOmega::getTupleAssignments(){
    return tupleAssignments;
}



void VisitorChangeUFsForOmega::preVisitSparseConstraints(
    SparseConstraints* sc) {
    if (sc->getNumConjuncts() != 1) {
        throw assert_exception(
            "Must have exactly one conjunction for Omega conversion");
    }
}
void VisitorChangeUFsForOmega::preVisitConjunction(Conjunction* c){
    bool requireChange = false;
    std::string prefix = "__x";
    std::list<Exp*> newConstraints;
    TupleDecl decl = c->getTupleDecl();
    for(unsigned int i = 0; i <decl.getSize(); i++){
        if(decl.elemIsConst(i)){
	    requireChange = true;
	    decl.setTupleElem(i,prefix+std::to_string(i));
	    // Create constraint for replacement
	    TupleVarTerm* tupleTerm = new TupleVarTerm(i);
	    Term* constTerm = new Term(-decl.elemConstVal(i));
	    Exp * eqConstraint = new Exp();
	    eqConstraint->setEquality();
            eqConstraint->addTerm(tupleTerm);
            eqConstraint->addTerm(constTerm);
	    newConstraints.push_back(eqConstraint);
	}
    }

    // Change the conjunction at this point to the
    // new conjunction.
    if (requireChange){
        Conjunction* conj = new Conjunction(decl);
        conj->setInArity(c->inarity());
	for(auto it = newConstraints.begin();
		it!=newConstraints.end(); it++){
	    conj->addEquality(*it);
	}
        for (auto it = c->equalities().begin();
		it!=c->equalities().end(); it++){
	    conj->addEquality((*it)->clone());
	}

        for (auto it = c->inequalities().begin();
		it!=c->inequalities().end(); it++){
	    conj->addInequality((*it)->clone());
	}
	if(c->isUnsat()){
            conj->setUnsat();
	}
        // Copy contents of new conjunction to current
	// conjunction being visited.
	*c = *conj;
	delete conj;
    }
    currentTupleDecl = decl;
    // Initilize all tuple variable to zero
    for (unsigned int i = 0; i < decl.size(); i++ ){
       tupleAssignments[i] = "0";
    }
}
void VisitorChangeUFsForOmega::preVisitExp(iegenlib::Exp * e){
    // The goal of this code section is to generate tuple
    // variable intiializations. The code is currently buggy
    // and will be bypassed for now.
    /*
    // Make sure the expression does not have UF
    // and is equality.
    if(!e->hasIndexedUFCall() && e->isEquality()){
        std::cerr << "Analysing: " << e->toString() << "\n";
	// Check if expression has only one tuple variable.
	TupleVarTerm * term = NULL;
	bool isValid = true;
	std::list<Term*> terms = e->getTermList();
	// Count number of tuple variable terms in expression,
	// multiple tuple term in an expression is not considered
	// as a candidate for tuple initialization.
	int tv_count = std::count_if(terms.begin(), terms.end(),
	    [](Term* t){return dynamic_cast<TupleVarTerm*>(t)!=NULL;});
	if (tv_count != 1){
	    return;
	}
	for(auto it = terms.begin();!term && it != terms.end(); it++){
	   if((*it)->isUFCall()){
	       isValid = false;
	       break;
	   }
	   term = dynamic_cast<TupleVarTerm*>(*it);
	}
	// If this expression is valid for assignment
	// we go ahead.
	if(isValid && term){
	    std::cerr << "Is Valid: " << term->toString() << "\n" ;
	    if(terms.size()==1){
	        tupleAssignments[term->tvloc()] = "0";
	        return;
	    }
	    term->setCoefficient(1);
	    Exp * solveFor = e->solveForFactor(term);
	    std::string solvedForString = solveFor->toString();
	    tupleAssignments[term->tvloc()] =  solvedForString;
	    delete solveFor;
	}
    }*/
}


void VisitorChangeUFsForOmega::postVisitUFCallTerm(UFCallTerm* callTerm) {
    if (currentTupleDecl == NULL) {
        throw assert_exception(
            "No TupleDecl collected -- is this Visitor (incorrectly) being run "
            "on something other than a Set/Relation?");
    }

    // determine which tuple variables are needed in the call (how large of a
    // prefix)
    int max_tvloc = -1;
    for (unsigned int i = 0; i < callTerm->numArgs(); ++i) {
        // loop through all terms, processing as needed
        for (const auto& term : callTerm->getParamExp(i)->getTermList()) {
            if (term->isUFCall()) {
                throw assert_exception("Nested UF calls are not yet supported");
            } else if (term->type() == "TupleVarTerm") {
                TupleVarTerm* termAsTupleVar = static_cast<TupleVarTerm*>(term);
                max_tvloc = std::max(termAsTupleVar->tvloc(), max_tvloc);
            } else if (term->type() == "TupleExpTerm") {
                throw assert_exception("TupleExpTerm unsupported");
            }
        }
    }
    // ensure presence of at least one tuple var (UF calls cannot be
    // constant-only)
    if (max_tvloc == -1) {
        throw assert_exception(
            "Cannot make UF calls with only constant arguments");
    }

    // save original coefficient, then temporarily modify for printing
    int originalCoefficient = callTerm->coefficient();
    callTerm->setCoefficient(1);
    std::string originalCall = callTerm->toString();

    // rewrite argument list as a prefix of input tuple
    callTerm->resetNumArgs(max_tvloc + 1);
    for (int i = 0; i < callTerm->numArgs(); ++i) {
        Exp* newParamExp = new Exp();
        TupleVarTerm* tupleVarParam = new TupleVarTerm(i);
        newParamExp->addTerm(tupleVarParam);
        callTerm->setParamExp(i, newParamExp);
    }

    std::string replacementName;
    auto it = knownUFs.find(originalCall);
    // check if this particular UF invocation has already been encountered
    if (it != knownUFs.end()) {
        // use the function name from the already-existing definition
        replacementName = it->second;
        callTerm->setName(replacementName);
    } else {
        // assign a new name to this function and add it to our list
        replacementName =
            callTerm->name() + "_" + std::to_string(nextFuncReplacementNumber);
        nextFuncReplacementNumber++;
        knownUFs.emplace(originalCall, replacementName);

        callTerm->setName(replacementName);
        // this is a new UF, so add a macro definition for it
        macros.emplace(callTerm->toString(), originalCall);
    }
    // add UF call to the list of declarations
    ufCallDecls.emplace(callTerm->name() + "(" + std::to_string(max_tvloc + 1) +
                        ")");

    // restore coefficient, which was changed temporarily for printing
    callTerm->setCoefficient(originalCoefficient);
}

}  // namespace iegenlib
