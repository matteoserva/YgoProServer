#include <vector>
#include <list>
#include <map>
namespace ygo
{
	struct QueueElement
	{
		std::vector<void* > elements;
		unsigned int queue;
	};
	
	
	
	
class QueueManager
{
	struct ElementInfo
	{
		unsigned int age;
		double score;
	};
	
	class ElementComparator
	{
		std::map<void*, ElementInfo> & map;
		public:
		ElementComparator(std::map<void*, ElementInfo> & map) : map(map){};
		bool operator()( void * el1, void  * el2)
		{
			if(map.find(el1) == map.end())
				printf("errore mappa\n");
			return map[el1].score > map[el2].score;
			
			
		}
		
		
	};
	
	unsigned int MAX_AGE;
	unsigned int MIN_AGE;
	
	
	std::map<void*, ElementInfo> elementsInfo;
	
	struct QueueData
	{
		std::list<void*> queuedElements;
		unsigned int group_size;
	};
	
	std::map<unsigned int, QueueData > queues;
	
	unsigned int queueSize(unsigned int);
	QueueElement queueExtractBest(unsigned int);
	std::list<void*>::iterator queueReadOldest(unsigned int);
	std::list<void*>::iterator queueReadNearest(unsigned int,double);
	
	public:
	QueueManager();
	void setMaxAge(unsigned int);
	bool createQueue(unsigned int id, unsigned int group_size);
	bool insertElement(void* element, double score, unsigned int queue);
	void removeElement(void* element);
	std::vector<QueueElement> extractBest();
	
	
};

}
