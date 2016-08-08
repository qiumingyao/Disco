/*
 * Read.cpp
 *
 * Created on: April 22, 2013
 * Author: Md. Bahlul Haider
 */

#include "Common.h"
#include "Read.h"



/**********************************************************************************************************************
	Default constructor
**********************************************************************************************************************/
Read::Read(void)
{
	// Initialize the variables.
	readNumber = 0;
	superReadID = 0;
	fileIndex=0;
	containmentChecked=false;
}

/**********************************************************************************************************************
	Another constructor
**********************************************************************************************************************/
Read::Read(UINT64 fIndx)
{
	readNumber = 0;
	superReadID = 0;
	fileIndex=fIndx;
	containmentChecked=false;
}

/**********************************************************************************************************************
	Default destructor
**********************************************************************************************************************/
Read::~Read(void)
{
	// delete all the pointers.
}
/**********************************************************************************************************************
	This function assigns an ID to the read.
**********************************************************************************************************************/
bool Read::setReadNumber(UINT64 id)
{
	if(id <= 0) MYEXIT("ID less than 1.");
	readNumber = id;												// Set the read number.
	return true;
}
/**********************************************************************************************************************
	This function assigns an ID to the read.
**********************************************************************************************************************/
void Read::setFileIndex(UINT64 id)
{
	if(id <= 0) MYEXIT("ID less than 1.");
	fileIndex = id;												// Set the read number.
}
/**********************************************************************************************************************
	Returns the reverse complement of a read.
**********************************************************************************************************************/

void Read::setReadHashOffset(UINT64 offset)
{
	readHashOffset=offset;
}




