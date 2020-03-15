#include <gtest/gtest.h>

#include <iegenlib.h>
using namespace iegenlib;
using namespace std;

/*!
 * \class Sparse Format Tests
 *
 * \brief Class to Sparse Format usage of IEGenLib.
 *
 * This class holds gtest test cases that test the functionality in
 * iegenlib that will be needed by CHILL.
 */
class SparseFormatTest : public::testing::Test {
    public:
    protected:
    virtual void SetUp() {}
    virtual void TearDown() {}
};




// TODO take to bottom
// Test for COO WRT Dense
TEST_F(SparseFormatTest, COOWRTDense_1) {
   Relation* r = 
	    new Relation("{[n] -> [i,j] : row(n) = i && col(n) = j "
			   "&& 0 <= n && n < NNZ && Dense(i,j) > 0}");
   EXPECT_EQ("{ [n] -> [i, j] : i - row(n) = 0"
             " && j - col(n) = 0 && n >= 0 && Dense(i, j) - 1"
	     " >= 0 && -n + NNZ - 1 >= 0 }",r->prettyPrintString());
   delete r;
}
// Test for CSR WRT DENSE
TEST_F(SparseFormatTest, CSRWRTDense_1) {
   Relation* r = 
	    new Relation("{[n] -> [i,j] :n >= rptr(i) "
			    "&& n < rptr(i+1) && col(n) = j "
			   "&& 0 <= n && n < NNZ && Dense(i,j) > 0}");
	   // UF for rptrcsr
   EXPECT_EQ("{ [n] -> [i, j] : j - col(n) = 0 && n >= 0 && n - rptr(i) >= 0 && Dense(i, j) - 1 >= 0 && -n + NNZ - 1 >= 0 && -n + rptr(i + 1) - 1 >= 0 }",r->prettyPrintString());
   delete r;
}

// Test for COO TO CSR COMPOSE
TEST_F(SparseFormatTest, COO_CSR_1) {
   Relation* r_coo = 
	    new Relation("{[n] -> [i,j] : row(n) = i && col(n) = j "
			   "&& 0 <= n && n < NNZ }");
   Relation* r_csr = 
	   new Relation("{[n] -> [i,j] :n >= rptr(i) "
			    "&& n < rptr(i+1) && rowcol_inv(i,j) = n "
			   "&& 0 <= n && n < NNZ }");
   // UF for rptrcsr
   //to be gotten using chill
   

   EXPECT_EQ("{ [n] -> [i, j] : n - rowcol_inv(i, j) = 0 && n >= 0 && n - rptr(i) >= 0 && -n + NNZ - 1 >= 0 && -n + rptr(i + 1) - 1 >= 0 }",
		   r_csr->prettyPrintString());

   EXPECT_EQ( "{ [n] -> [i, j] : i - row(n) = 0 && j - col(n) = 0 && n >= 0 && -n + NNZ - 1 >= 0 }",r_coo->prettyPrintString());
   Relation* r_csr_inv = r_csr->Inverse();
   EXPECT_EQ("{ [i, j] -> [n] : n - rowcol_inv(i, j) = 0 && n >= 0 && n - rptr(i) >= 0 && -n + NNZ - 1 >= 0 && -n + rptr(i + 1) - 1 >= 0 }"
		   ,r_csr_inv->prettyPrintString());
   Relation* r_coo_csr_comp = r_csr_inv->Compose(r_coo);
   EXPECT_EQ("{ [n] -> [n1] : n1 - rowcol_inv(row(n), col(n)) = 0 && n >= 0 && n1 >= 0 && n1 - rptr(row(n)) >= 0 && -n + NNZ - 1 >= 0 && -n1 + NNZ - 1 >= 0 && -n1 + rptr(row(n) + 1) - 1 >= 0 }",r_coo_csr_comp->prettyPrintString());
   
  
   delete r_coo;
   delete r_csr;
   delete r_csr_inv;
   delete r_coo_csr_comp;
}

TEST_F (SparseFormatTest, COO_BCSR){
   Relation* Flatten = 
	   new Relation("{ [n] -> [i,j] : n = i * R + j }");

   Relation* Zeros = 
	   new Relation ( " { [i,j] -> [i_p,j_p,i_pp,j_pp]:"
			   " Dense(i_p,j_p) = 0 && Dense(i_pp,j_pp) != 0 && "
			   " i_pp/R =i_p/R && j_pp/R = j_p/R}");
   Relation* coo = 
	    new Relation("{[n] -> [i,j] : row(n) = i && col(n) = j "
			 "&& 0 <= n && n < NNZ && Dense(i,j) != 0}");

   Relation * bcsr_inv =
	   new Relation("{ [i,j] -> [b,i_p,j_p]: 0 <= b && b < NB "
			" && b = block_inv(i/R,j/C) && 0 <= i_p   "
			" && i_p < R && 0 <= j_p < C && i_p =     "
			"i - (i/R) * R && j_p = j- (j/C) *C}      ");

   Relation * Zeros_Flatten = Zeros->Compose(Flatten);
   Relation* bcsr_coo = bcsr_inv->Compose(coo->Union(Zeros_Flatten));
   EXPECT_EQ("{ [n] -> [n1] : n1 - rowcol_inv(row(n), col(n)) = 0 && n >= 0 && n1 >= 0 && n1 - rptr(row(n)) >= 0 && -n + NNZ - 1 >= 0 && -n1 + NNZ - 1 >= 0 && -n1 + rptr(row(n) + 1) - 1 >= 0 }",bcsr_coo->prettyPrintString());
   



}
