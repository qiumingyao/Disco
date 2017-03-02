//============================================================================
// Name        : Edge.cpp
// Author      : Tae-Hyuk (Ted) Ahn, JJ Crosskey
// Version     : v1.2
// Copyright   : 2015 Oak Ridge National Lab (ORNL). All rights reserved.
// Description : Edge cpp file
//============================================================================

#include "Config.h"
#include "Edge.h"
#include <numeric>
extern TLogLevel loglevel;                      /* verbosity level of logging */


Edge::Edge(void)
{
	m_orient		= 0;
	m_overlapOffset		= 0;
	m_flag			= 0;
	m_string		= std::string();
	m_coverageDepth		= 0;
	m_SD			= 0;
	m_invalid = false;
	m_source=nullptr;
	m_destination=nullptr;
	m_listOfReads = nullptr;
	m_listSize = 0;
	m_flow=0;
	m_reverseEdge=nullptr;
}

// Constructor for a (composite) edge, given
Edge::Edge(Read *source, Read *destination, UINT8 orient, UINT32 overlapOffset,
		UINT64 *listOfReads, UINT32 listSize, UINT64 flowValue)
:m_source(source), m_destination(destination),m_reverseEdge(nullptr),m_listOfReads(listOfReads),
 m_coverageDepth(0.0), m_SD(0),m_string(std::string()),  m_listSize(listSize), m_overlapOffset(overlapOffset), m_orient(orient),
	m_flag(0), m_invalid(false), m_flow(flowValue)
{
	if(m_listOfReads != nullptr &&
		getInnerOverlapOffset(0) > m_source->getReadLength()){
		FILE_LOG(logERROR) << "ERROR: ("<<m_source->getReadID()<<","<<m_destination->getReadID()<<") overlap offset " << getInnerOverlapOffset(0)
			<< " is greater than source read length " << m_source->getReadLength() << "\n";
	}
	// If this edge is a simple edge, make its reverse edge
	// For composite edge, leave the reverse edge to the sum of reverse edges
	if(source == destination)
		m_flag |= (1 << 1);
}

Edge::Edge(Read *source, Read *destination, UINT8 orient, UINT32 overlapOffset, UINT64 flowValue)
:m_source(source), m_destination(destination), m_reverseEdge(nullptr), m_listOfReads(nullptr),
 m_coverageDepth(0.0), m_SD(0), m_string(std::string()),  m_listSize(0), m_overlapOffset(overlapOffset), m_orient(orient),
	m_flag(0), m_invalid(false), m_flow(flowValue)
{
	if(m_overlapOffset >= m_source->getReadLength()){
		FILE_LOG(logERROR) << "ERROR: overlap offset " << m_overlapOffset 
			<< " is greater than source read length " << m_source->getReadLength() << "\n";
	}
	// If this edge is a simple edge, make its reverse edge
	// For composite edge, leave the reverse edge to the sum of reverse edges
	if(source == destination)
		m_flag |= (1 << 1);
}

void Edge::copyEdge(const Edge &edge)
{
	//CLOCKSTART;
//	FILE_LOG(logDEBUG1) << edge << "\n";
	//FILE_LOG(logDEBUG1) << "Copy number variables\n";
	m_source = edge.m_source;
	m_destination = edge.m_destination;
	m_orient = edge.m_orient;
	m_overlapOffset = edge.m_overlapOffset;
	m_flag = edge.m_flag;
	m_string = edge.m_string;
	m_coverageDepth = edge.m_coverageDepth;
	m_SD = edge.m_SD;
	m_flow = edge.m_flow;
	m_invalid  = edge.m_invalid;
	m_listSize=edge.m_listSize;
	//FILE_LOG(logDEBUG1) << "Copy list of reads\n";
	if(edge.m_listOfReads){
		m_listOfReads = new UINT64[edge.m_listSize];
		for(size_t i=0; i < m_listSize; i++)
			m_listOfReads[i]=edge.m_listOfReads[i];
	}
	else 
		m_listOfReads = nullptr;

}

Edge::Edge(const Edge &edge)
{
	copyEdge(edge);
	m_reverseEdge = new Edge;
	m_reverseEdge->copyEdge(*(edge.m_reverseEdge));
	m_reverseEdge->setReverseEdge(this);
}

Edge& Edge::operator=(const Edge &edge)
{
	if(this == &edge)
		return *this;
	clearEdge();
	//CLOCKSTART;
	copyEdge(edge);
	m_reverseEdge = new Edge;
	m_reverseEdge->copyEdge(*(edge.m_reverseEdge));
	m_reverseEdge->setReverseEdge(this);
	//CLOCKSTOP;
	return *this;
}

Edge::~Edge()
{
	delete[] m_listOfReads;
	m_listOfReads = nullptr;
}
void Edge::clearEdge()
{
	delete[] m_listOfReads;
	m_listOfReads = nullptr;
	// This function does not delete the reverse edge
}


/* Get the overlap length for the first link in the edge */
UINT32 Edge::getOverlapLen() const
{
	if(!m_listOfReads || m_listSize==0)
		return (m_source->getReadLength() - m_overlapOffset);
	else
		return (m_source->getReadLength() - getInnerOverlapOffset(0));
}

UINT32 Edge::getInnerOverlapSum(size_t start, size_t end) const
{
	UINT32 overlap_sum=0;
	for(size_t i=start;i<m_listSize && i<end;i++)
		overlap_sum += getInnerOverlapOffset(i);
	return overlap_sum;
}

/* Get the overlap length for the last link in the edge */
UINT32 Edge::getLastOverlapOffset() const
{
	if(!m_listOfReads || m_listSize==0)
		return m_overlapOffset;
	UINT32 overlap_sum = getInnerOverlapSum(0,m_listSize);
	return (m_overlapOffset - overlap_sum);
}

std::string Edge::getEdgeStringWithoutSource() const
{
	return m_string.substr(m_source->getReadLength());
}
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  breakForwardEdge
 *  Description:  Break an edge at a specified link, only the forward edge
 * =====================================================================================
 */
vector<Edge*> Edge::breakForwardEdge(const UINT32 &link, DataSet *d) const
{
	// sub edges generated by breaking a link from an edge
	vector<Edge*> sub_edges;

	/* If the edge is simple, no new edge is generated, return empty vector.
	 * If the edge is composite, insert two new edges, and then delete the current edge */
	if ( !m_listOfReads && m_listSize==0){

		UINT64 num_reads = m_listSize;	// number of contained reads

		assert(link <= num_reads);// Link index should not exceed number of contained reads

		/* the position is not the first link, 
		 * there will be an edge with source the same as the original source */
		if(link != 0){                   
			UINT32 ovloffset = getInnerOverlapOffset(0);
			// Make the edge before the link
			Read* e1_dest_read =  d->at(getInnerReadID(link-1));
			UINT8 e1_orient = (m_orient & 2) + getInnerOrientation(link-1);
			UINT64 * e1_list_reads = nullptr;
			UINT32 lSize=0;
			/* breaking at link 1, e1 is a simple edge */
			/* breaking after link 1, e1 is a composite edge */
			if(link > 1){
				lSize = link-1;
				e1_list_reads = new UINT64[lSize];
				UINT64 i = 0;
				for(i = 0; i < (link-1); i++){
					e1_list_reads[i] = m_listOfReads[i];
					ovloffset += getInnerOverlapOffset(i+1);
				}
			}
			Edge * e1 = new Edge(m_source, e1_dest_read, e1_orient, 
					ovloffset, e1_list_reads, lSize);
			sub_edges.push_back(e1);
		}
		/* the position is not the last link, 
		 * there will be an edge with destination the same as the original destination */
		if(link != num_reads){          
			UINT32 ovloffset = getLastOverlapOffset(); /* last overlap offset */
			Read* e2_source_read = d->at(getInnerReadID(link));
			UINT8 e2_orient = (m_orient & 1) + (getInnerOrientation(link)  << 1);
			UINT64 * e2_list_reads = nullptr;
			UINT32 lSize=0;
			/* broken at the second last link,
			 * e2 is a simple edge */
			/* e2 is a composite edge */
			if(link != (num_reads-1)){
				lSize=num_reads-link-1;
				e2_list_reads = new UINT64[lSize];
				UINT64 i = 0;
				for(i = link+1; i < num_reads; i++){
					e2_list_reads[i] = m_listOfReads[i];
					ovloffset += getInnerOverlapOffset(i);
				}
			}
			Edge *e2 = new Edge(e2_source_read, m_destination, e2_orient, ovloffset, e2_list_reads,lSize);
			sub_edges.push_back(e2);
		}
	}
	return sub_edges;
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  breakEdge
 *  Description:  break a given edge into (at most) 2 bi-directed edges.
 *  First create sub-edges after the specified link is taken out, both forward and reverse;
 *  then insert these sub-edges in the graph, then remove the original edge.
 * =====================================================================================
 */
vector<Edge*> Edge::breakEdge(const UINT32 &link, DataSet *d) const
{
	vector<Edge*> sub_edges;
	if(m_listOfReads && m_listSize!=0)
	{
		/* get new forward subedges */
		vector<Edge*> forward_sub_edges = this->breakForwardEdge(link,d);
		/* get new backward subedges */
		vector<Edge*> backward_sub_edges = m_reverseEdge->breakForwardEdge(m_listSize - link, d);
		UINT64 num_edges = forward_sub_edges.size();
		for(UINT64 i = 0; i < num_edges; i++) {
			// set twin edges
			forward_sub_edges.at(i)->setReverseEdge(backward_sub_edges.at(num_edges-i-1));
			backward_sub_edges.at(num_edges-i-1)->setReverseEdge(forward_sub_edges.at(i));
			sub_edges.push_back(forward_sub_edges.at(i));
		}
	}
	return sub_edges;
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  getBaseByBaseCoverage
 *  Description:  Calculate the coverage depth of an edge for every basepair and then update
 *                the Mean and SD of coverage depth in the edge. Only consider reads that are
 *                unique to the edge.
 * =====================================================================================
 */
void Edge::updateBaseByBaseCoverageStat(DataSet *d)
{
	// Array length is the same as the std::string length in the edge.
	UINT64 length = getEdgeLength();
	vector<UINT64> coverageBaseByBase;
	coverageBaseByBase.reserve(length);

	for(UINT64 i = 0; i < length; i++) {
		coverageBaseByBase.push_back(0);	// At first all the bases are covered 0 times.
	}
	UINT64 overlapOffset(0);

	// Increment the coverage of the section that each read covers,
	// NOT counting the source read and destination read because they are shared among multiple edges
	// JJ: so? if they are shared, their coverage should just be discarded?
	//     If indeed discarded, then the coverage vector can be shorter
	if(m_listOfReads){
		for(UINT64 i = 0; i < m_listSize; i++)	// For each read in the edge.
		{
			UINT64 rID = getInnerReadID(i);
			Read *read = d->at(rID);
			// Where the current read starts in the std::string.
			UINT32 readOffset = getInnerOverlapOffset(i);
			overlapOffset += readOffset;
			for(UINT64 j = overlapOffset; j < (overlapOffset + read->getReadLength()) && j<length; ++j) {
				coverageBaseByBase.at(j) += d->getReadCoverage(rID, j-overlapOffset);
			}
		}
	}
	m_coverageDepth = get_mean(coverageBaseByBase);
	m_SD = get_sd(coverageBaseByBase);
}

/*
 * ===  FUNCTION  ======================================================================
 *         Name:  getBaseByBaseCoverage
 *  Description:  Calculate the coverage depth of an edge for every basepair and return the
 *  				coverage values.
 * =====================================================================================
 */
vector<UINT64> Edge::getBaseByBaseCoverageValues(DataSet *d)
{
	// Array length is the same as the std::string length in the edge.
	UINT64 length = getEdgeLength();
	vector<UINT64> coverageBaseByBase;
	coverageBaseByBase.reserve(length);

	for(UINT64 i = 0; i < length; i++) {
		coverageBaseByBase.push_back(0);	// At first all the bases are covered 0 times.
	}
	UINT64 overlapOffset(0);

	// Increment the coverage of the section that each read covers,
	// NOT counting the source read and destination read because they are shared among multiple edges
	// JJ: so? if they are shared, their coverage should just be discarded?
	//     If indeed discarded, then the coverage vector can be shorter
	if(m_listOfReads){
		for(UINT64 i = 0; i < m_listSize; i++)	// For each read in the edge.
		{
			UINT64 rID = getInnerReadID(i);
			Read *read = d->at(rID);
			// Where the current read starts in the std::string.
			UINT32 readOffset = getInnerOverlapOffset(i);
			overlapOffset += readOffset;
			for(UINT64 j = overlapOffset; j < (overlapOffset + read->getReadLength()) && j<length; ++j) {
				coverageBaseByBase.at(j) += d->getReadCoverage(rID, j-overlapOffset);
			}
		}
	}
	return coverageBaseByBase;
}

//=============================================================================
// Merge two edges in the overlap graph.
//=============================================================================
Edge* Add( const Edge *edge1, const Edge *edge2)
{
	assert (is_mergeable(edge1, edge2));
	Edge *merge_forward = merge_forward_edges(*edge1, *edge2);
	Edge *merge_reverse = merge_forward_edges(*(edge2->m_reverseEdge), *(edge1->m_reverseEdge));
	merge_forward->setReverseEdge(merge_reverse);
	merge_reverse->setReverseEdge(merge_forward);
	return merge_forward;
}
void Edge::setReverseEdge(Edge * edge)
{ 
	if(m_reverseEdge == edge)
		return;
	else if(!m_reverseEdge) {
		m_reverseEdge = edge;
	}
	else{
		m_reverseEdge = edge;
	}
}

// Assisting function for just merging forward edges
Edge*  merge_forward_edges(const Edge & edge1, const Edge & edge2)
{

	// Orientation
	UINT8 orientationForward = mergedEdgeOrientation(edge1.m_orient, edge2.m_orient);

	// overlap offset
	UINT32 overlapOffsetForward = edge1.m_overlapOffset + edge2.m_overlapOffset;

	UINT64 *listReadsForward = nullptr;
	UINT64 lSize=0;

	// Merge the lists from the two edges.
	mergeList(&edge1, &edge2, &listReadsForward, lSize);

	// Make the forward edge
	Edge *edgeForward = new Edge(edge1.m_source,edge2.m_destination,orientationForward, 
			overlapOffsetForward, listReadsForward, lSize);

	return edgeForward;
}

//=============================================================================
// Merge the list of reads, list of overlap offsets and list of orientations of two edges.
//=============================================================================
void mergeList(const Edge *edge1, const Edge *edge2, 
		UINT64 **returnListReads, UINT64 &lSize)
{
//	CLOCKSTART;
	lSize=edge1->getListofReadsSize()+edge2->getListofReadsSize()+1;
	size_t lCtr=0;
	UINT64 *listReads = new UINT64[lSize];
	// Copy the list from edge1.
	if (edge1->isListofReads() && !(edge1->getListofReadsSize()==0) ){
		for(size_t i = 0; i < edge1->getListofReadsSize(); ++i){
			listReads[lCtr++]=edge1->getInnerReadInfo(i);
		}
	}
	UINT64 overlapOff = (edge1->getLastOverlapOffset() << 32);
	UINT64 orient = (edge1->getOrientation() & 1);
	orient = orient << 63;
	// Insert the common node of the two edges
	UINT64 rID =  edge1->getDestinationRead()->getReadID() | overlapOff | orient;
	listReads[lCtr++] = rID;

	// Concatenate the list from edge2.
	if (edge2->isListofReads() && !(edge2->getListofReadsSize()==0) ){
		for(size_t i = 0; i <edge2->getListofReadsSize(); ++i){
			listReads[lCtr++]=edge2->getInnerReadInfo(i);
		}
	}
	*returnListReads = listReads;
//	CLOCKSTOP;
}

//=============================================================================
// Create the forward list of reads, list of overlap offsets and list of orientations from the string of edges in the file.
//=============================================================================
UINT64 createFwdList(string readList,UINT64 **returnListReads, UINT64 &lSize,UINT64 & unUsedMate, DataSet *d)
{
	UINT64 usedReads=0;
	if(readList.length()>0)
	{
		//The reads are in ordered triplets (readID, orientation, offset)
		//Split based on ')'
		vector<string> readTok =  Utils::split(readList,')');
		lSize=readTok.size();
		size_t lCtr=0;
		UINT64 *listReads = new UINT64[lSize];
		for(size_t i=0;i<readTok.size();i++)
		{
			vector<string> processStr = Utils::split(readTok[i].substr(1),','); //Get rid of the '(' at the beginning and split on ','
			UINT64 readID = std::stoull(processStr[0],nullptr,0);
			UINT64 orient = std::stoull(processStr[1],nullptr,0);
			orient = orient << 63;
			UINT64 overlapOff = std::stoull(processStr[2],nullptr,0);
			overlapOff = overlapOff << 32;
			UINT64 packedReadInfo =  readID | overlapOff | orient;
			listReads[lCtr++] = packedReadInfo;

			//Check if mate exists and has been used in previous assembly
			UINT64 readIDMate = d->getMatePair(readID);
			if(d->at(readID)->isUsedRead()) //Check if read has been used
				usedReads++;

			if(readIDMate > 0)		//Mate exists
			{
				if(!d->at(readIDMate)->isUsedRead()) //Check if mate has not been used
					unUsedMate++;
			}

			/*if(readIDMate > 0)		//Mate exists
			{
				if(d->at(readID)->isUsedRead() && d->at(readIDMate)->isUsedRead()) //Check if both have been used
					usedReads++;
			}
			else
			{
				if(d->at(readID)->isUsedRead()) //Check if read has been used
					usedReads++;
			}*/
		}
		*returnListReads = listReads;
	}
	return usedReads;
//	CLOCKSTOP;
}

//=============================================================================
// Create the reverse list of reads, list of overlap offsets and list of orientations from the string of edges in the file.
//=============================================================================
void createRevList(Edge *forwardEdge, UINT64 **returnListReads, UINT64 &lSize, DataSet *d)
{
	UINT64 overlapOffsetSum=0;
	if(forwardEdge->isListofReads())
	{
		lSize=forwardEdge->getListofReadsSize();
		size_t lCtr=0;
		UINT64 *listReads = new UINT64[lSize];
		for(size_t i=0;i<lSize;i++)
		{
			size_t revIndex = (lSize-1)-i;
			UINT64 readID = forwardEdge->getInnerReadID(revIndex);
			UINT8 orient = forwardEdge->getInnerOrientation(revIndex);
			if(orient==1)
			{
				orient = 0;
			}
			else
			{
				orient=1;
			}
			UINT64 orientRev = ((UINT64)orient) << 63;
			UINT64 fwdOffsetSum=forwardEdge->getInnerOverlapSum(0,revIndex+1);
			UINT64 overlapOff =  forwardEdge->getEdgeLength() - d->at(readID)->getReadLength() - fwdOffsetSum - overlapOffsetSum;
			overlapOffsetSum+=overlapOff;
			overlapOff = overlapOff << 32;
			UINT64 packedReadInfo =  readID | overlapOff | orientRev;
			listReads[lCtr++] = packedReadInfo;
		}
		*returnListReads = listReads;
	}
}


//=============================================================================
// Check if two edges can be merged into one edge
// For two edges e1(u,v) and e2(v,w), at node v, 
// one of the edges should be an incoming edge and the other should be an outgoing
// edge to match.
//=============================================================================
bool is_mergeable(const Edge *edge1, const Edge *edge2)
{
	// First, the destination of edge1 has to be the same as 
	// the source read of edge2
	if (edge1->getDestinationRead()->getReadID() != edge2->getSourceRead()->getReadID()){
		return false;
	}

	// *-----> and >------* *1 and 1*
	// *-----< and <------* *0 and 0*
	else if ((edge1->getOrientation() & 1) == ((edge2->getOrientation() >>1) & 1))
		return true;
	else{
		return false;
	}
}

// Orientation of the edge when two edges are merged.
UINT8 mergedEdgeOrientation(const UINT8 &orient1, const UINT8 &orient2)
{
//	assert(orient1 < 4 && orient2 < 4);
	return ((orient1 & 2) | (orient2 & 1));
}

UINT8 get_twin_orient(const UINT8 &orient)
{
	assert(static_cast<int>(orient) < 4); // Quit if orient is not 0, 1, 2, or 3
	// Exchange the last two bits of the orient, and flip them
	UINT8 twin_orient = ((orient >> 1) ^ 1) | (((orient & 1) ^ 1) << 1) ;
	assert(static_cast<int>(twin_orient) < 4);
	return twin_orient;
}


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  loadReadString
 *  Description:  Fill the string for the current edge from a read and its string
 * =====================================================================================
 */
void Edge::loadReadString(const std::string read_str, int index)
{
	// If this edge is not smaller edge ( source smaller than the destination ID)
	// or its length is smaller than the reporting threshold,
	// do not load string in this edge.
	//if (!isSmallerEdge()){
	//	return;
	//}
	// TODO: need to update the mismatch base count
	assert(index >= -2);
	if(m_listOfReads)
		assert(index < static_cast<int>(m_listSize));
	if(m_string.length()==0)
		m_string = std::string(getEdgeLength(), 'N');
    // Read is the source of the edge, get the first overlap offset length
	if(index == -1){
		UINT32 len = getFirstOverlapOffset();
		m_string.replace(0, len, read_str, 0, len);
	}
	// Read is the destination of the edge
	else if (index == -2){
		m_string.replace(m_overlapOffset, read_str.length(), read_str,
				0, read_str.length());
	}
	// Read is neither source nor destination, but lives on the edge
	else{
		UINT32 start;
		UINT32 len;
		if(index < static_cast<int>(m_listSize - 1))  // Not the last read on edge
		{
			len = getInnerOverlapOffset(index+1);
			start = getInnerOverlapSum(0,index+1);
		}
		else {
			len = getLastOverlapOffset();
			start = getInnerOverlapSum(0,m_listSize);
		}
		if(len<read_str.size())
			m_string.replace(start, len, read_str, 0, len);
		else
			m_string.replace(start, read_str.length(), read_str, 0, read_str.length());
	}
	if(m_string.length() != getEdgeLength()){
		FILE_LOG(logERROR) << "ERROR: edge length changed!!! edge: "<< m_source->getReadID() <<" to "<< m_destination->getReadID() << " on index " << index << " from length " << m_string.length() << " and " << getEdgeLength() << "\n";
	}
}


template <typename T>
float get_mean(const vector<T> &numbers)
{
	if (numbers.empty())
		return 0;
	float sum(0);
	sum = std::accumulate(numbers.begin(), numbers.end(), sum);
	return (sum/static_cast<float>(numbers.size()));
}

template <typename T>
float get_sd(const vector<T> &numbers)
{
	if(numbers.empty())
		return 0;
	float mean = get_mean(numbers);
	float variance(0.0f);
	for(auto it = numbers.begin(); it != numbers.end(); ++it){
		variance += (*it - mean) * (*it - mean);
	}
	return (sqrt(variance/numbers.size()));
}

bool isSameCompositePath(const Edge &subject, const Edge &query)
{
	if(subject.isListofReads() && query.isListofReads())
	{
		if(subject.getListofReadsSize()!=query.getListofReadsSize())
			return false;
		else
		{
			for(size_t i=0;i<subject.getListofReadsSize();i++)
			{
				UINT64 sRID = subject.getInnerReadID(i);
				UINT64 dRID = query.getInnerReadID(i);
				if(sRID!=dRID)
					return false;
			}
		}
		return true;
	}
	else if(!subject.isListofReads() && !query.isListofReads())
		return true;
	return false;
}

/*
 * Compares source, destination, orientation, overlap and path of the edges
 *
 */
bool operator==(const Edge &subject, const Edge &query)
{
	if(subject.getSourceRead()->getReadID() == query.getSourceRead()->getReadID() &&
			subject.getDestinationRead()->getReadID() == query.getDestinationRead()->getReadID() &&
			subject.m_overlapOffset == query.m_overlapOffset &&
			subject.m_orient == query.m_orient &&
			isSameCompositePath(subject, query))
		return true;
	return false;
}

/*
 * Add edge to the edge list of a read
 */
void Read::setEdge(Edge *edge, UINT32 readIndx, UINT32 orient)
{
	if(noOfAllocEdgeMemAvail>noOfEdges)
	{
		noOfEdges++;
		edgeP[noOfEdges-1] = edge;
		orient = orient << 31;
		UINT64 newOriIndx =  readIndx | orient;
		edgeOriIndex[noOfEdges-1] = newOriIndx;
	}
	else
	{
		noOfEdges++;
		noOfAllocEdgeMemAvail++;
		Edge **newEdgeArray = new Edge*[noOfEdges];
		UINT32 *newEdgeIndexArray = new UINT32[noOfEdges];
		for(UINT16 i=0;i<(noOfEdges-1);i++)
		{
			newEdgeArray[i] = edgeP[i];
			newEdgeIndexArray[i] = edgeOriIndex[i];
		}
		newEdgeArray[noOfEdges-1] = edge;
		orient = orient << 31;
		UINT64 newOriIndx =  readIndx | orient;
		newEdgeIndexArray[noOfEdges-1] = newOriIndx;
		delete[] edgeP;
		delete[] edgeOriIndex;
		edgeP = newEdgeArray;
		edgeOriIndex = newEdgeIndexArray;
	}
}

