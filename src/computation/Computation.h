/*!
 * \file Computation.h
 *
 * \brief Declarations for the Computation class, and supporting classes.
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

#ifndef COMPUTATION_H_
#define COMPUTATION_H_

#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "set_relation/Visitor.h"
#include "set_relation/environment.h"
#include "set_relation/expression.h"
#include "set_relation/set_relation.h"

namespace iegenlib {

class Stmt;

/*!
 * \class Computation
 *
 * \brief SPF representation of a computation (group of statements such as a
 * function).
 */
class Computation {
   public:
    //! Construct an empty Computation
    Computation();

    //! Destructor
    ~Computation();

    //! Copy constructor
    Computation(const Computation& other);

    //! Assignment operator (copy)
    Computation& operator=(const Computation& other);

    //! Equality operator
    bool operator==(const Computation& other) const;

    //! Get a copy of this Computation with uniquely-prefixed data spaces
    //! Uses string find-and-replace to change data space names, with the
    //! assumption that incoming names will be unique enough for this to work
    //! without unintended replacements occuring.
    Computation* getDataPrefixedCopy();

    //! Add a statement to this Computation.
    //! Statements are numbered sequentially from 0 as they are inserted.
    //! @param[in] stmt Stmt to add (adopted)
    void addStmt(Stmt* stmt);
    //! Get a statement by index
    Stmt* getStmt(unsigned int index);

    //! Add a data space to this Computation
    void addDataSpace(std::string dataSpaceName);
    //! Get data spaces
    std::unordered_set<std::string> getDataSpaces() const;

    //! Add a parameter to this Computation
    void addParameter(std::string parameterName);
    //! Get parameters
    std::vector<std::string> getParameterNames() const;

    //! Get the number of statements in this Computation
    unsigned int getNumStmts() const;

    //! Get an iterator to the first Stmt in this Computation
    std::vector<Stmt*>::iterator stmtsBegin() { return stmts.begin(); }
    //! Get an iterator pointing past the last Stmt in this Computation
    std::vector<Stmt*>::iterator stmtsEnd() { return stmts.end(); }

    //! Print out all the information represented in this Computation for
    //! debug purposes
    void printInfo() const;

    //! Get whether this Computation's required information is filled out,
    //! including that of the Stmts it contains
    bool isComplete() const;

    //! Clear all data from this Computation
    void clear();

    //! Add statements from another Computation to this one for inlining.
    //! @param[in] other Computation to append to this one
    //! @param[in] level 0-indexed execution schedule tuple position to append
    //! at, to define the level of nesting the new Computation is within
    int appendComputation(Computation* other, unsigned int level = 0);

    void toDot(std::fstream& dotFileStream, string fileName);

    //! Environment used by this Computation
    Environment env;

    //! Method generates c code.
    //! Known constraints are constraints that can be considered already
    //! enforced/are known to be true without need for checking in generated
    //! code (guards).
    std::string codeGen(Set* knownConstraints = nullptr);

    //! Method returns omega strings for each statement
    std::string toOmegaString();

   private:
    //! Information on all statements in the Computation
    std::vector<Stmt*> stmts;
    //! Data spaces accessed in the computation
    std::unordered_set<std::string> dataSpaces;
    //! Parameters of the computation (not included in data spaces)
    std::vector<std::string> parameterNames;

    //! Number of times this Computation has been called by (appended on to)
    //! others, monitored for name mangling
    unsigned int numRenames = 0;
};

//! Prints the dotFile for the Computation structure

/*!
 * \class Stmt
 *
 * \brief Information attached to a statement represented as mathematical
 * objects.
 */
class Stmt {
   public:
    //! Construct an empty Stmt
    Stmt(){};

    //! Destructor
    ~Stmt();

    //! Construct a complete Stmt, given strings that will be used to
    //! construct each set/relation.
    Stmt(std::string stmtSourceCode, std::string iterationSpaceStr,
         std::string executionScheduleStr,
         std::vector<std::pair<std::string, std::string>> dataReadsStrs,
         std::vector<std::pair<std::string, std::string>> dataWritesStrs);

    //! Copy constructor
    Stmt(const Stmt& other);

    //! Assignment operator (copy)
    Stmt& operator=(const Stmt& other);

    //! Equality operator
    //! Checks equality, NOT mathematical equivalence
    bool operator==(const Stmt& other) const;

    //! Get a copy of this Stmt with the given prefix applied to all data space
    //! names
    Stmt* getDataPrefixedCopy(std::string prefix);

    //! Get whether or not all necessary information for this Stmt is set
    bool isComplete() const;

    //! Get the source code of the statement
    std::string getStmtSourceCode() const;
    //! Set the source code of the statement
    void setStmtSourceCode(std::string newStmtSourceCode);

    //! Get the iteration space Set
    Set* getIterationSpace() const;
    //! Set the iteration space, constructing it from the given string
    void setIterationSpace(std::string newIterationSpaceStr);

    //! Get the execution schedule Relation
    Relation* getExecutionSchedule() const;
    //! Set the execution schedule, constructing it from the given string
    void setExecutionSchedule(std::string newExecutionScheduleStr);

    //! Add a data read
    void addRead(std::string dataSpace, std::string relationStr);
    //! Get the number of data reads for this statement
    unsigned int getNumReads() const;
    //! Get a data read's data space by index
    std::string getReadDataSpace(unsigned int index) const;
    //! Get a data read's relation by index
    Relation* getReadRelation(unsigned int index) const;

    //! Add a data write
    void addWrite(std::string dataSpace, std::string relationStr);
    //! Get the number of data writes for this statement
    unsigned int getNumWrites() const;
    //! Get a data write's data space by index
    std::string getWriteDataSpace(unsigned int index) const;
    //! Get a data write's relation by index
    Relation* getWriteRelation(unsigned int index) const;

   private:
    //! Source code of the statement, for debugging purposes
    std::string stmtSourceCode;
    //! Iteration space of a statement
    std::unique_ptr<Set> iterationSpace;
    //! Execution schedule of a single statement
    std::unique_ptr<Relation> executionSchedule;
    //! Read dependences of a statement, pairing data space name to relation
    std::vector<std::pair<std::string, std::unique_ptr<Relation>>> dataReads;
    //! Write dependences of a statement, pairing data space name to relation
    std::vector<std::pair<std::string, std::unique_ptr<Relation>>> dataWrites;
};

/*!
 * \class VisitorChangeUFsForOmega
 *
 * \brief Visitor that makes modifications to UF call terms so that they are
 * acceptable for what Omega supports.
 *
 * Intended for use on sets/relations. Also gathers information needed to pass
 * along to Omega codegen, such as #define macros.
 */
class VisitorChangeUFsForOmega : public Visitor {
   private:
    //! UF call (re)definition macros
    std::map<std::string, std::string> macros;
    //! declarations of UF calls needed by Omega parser
    std::set<std::string> ufCallDecls;
    //! UF calls that we've seen before and do not need to rename if encountered
    //! again; mapping from call as string -> new assigned name
    std::map<std::string, std::string> knownUFs;
    //! next number to use in creating unique function names
    int nextFuncReplacementNumber;
    //! stored tuple decl for variable retrieval
    TupleDecl currentTupleDecl;

   public:
    //! Construct a new VisitorChangeUFsForOmega
    VisitorChangeUFsForOmega();

    //! Destructor
    ~VisitorChangeUFsForOmega();

    //! Completely reset state of Visitor for re-use, including freeing memory.
    //! This method should NOT be used between visiting connected
    //! Sets/Relations, for instance those which share some UF calls you want
    //! the Visitor to remember.
    void reset();

    //! Partially reset Visitor state so that it is prepared for use with
    //! additional interrelated Sets/Relations, such as statements in a
    //! Computation.
    void prepareForNext();

    //! Get the UF call macros required for the code corresponding to the
    //! set/relation to function correctly, as a string
    std::map<std::string, std::string>* getUFMacros();

    //! Get the declarations of UF calls needed by Omega parser
    std::set<std::string> getUFCallDecls();

    void preVisitSparseConstraints(SparseConstraints*);

    void preVisitConjunction(Conjunction* c);

    void postVisitUFCallTerm(UFCallTerm*);
};

}  // namespace iegenlib

#endif
