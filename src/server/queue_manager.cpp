#include "queue_manager.h"
#include <cmath>
#include <algorithm>
namespace ygo
{
	
	QueueManager::QueueManager()
	{
		MAX_AGE=5;
		MIN_AGE=2;		
	}
	
	void QueueManager::setMaxAge(unsigned int m)
	{
		MAX_AGE=m;
		
	}
	
	bool QueueManager::createQueue(unsigned int id, unsigned int group_size)
	{
		if(queues.find(id) != queues.end())
		return false;
		
		QueueData qd;
		qd.group_size = group_size;
		queues[id] = qd;
		return true;
		
	}
	bool QueueManager::insertElement(void* element, double score, unsigned int queue)
	{
		
		
		if(queues.find(queue) == queues.end())
			return false;
		
		std::list<void*> & queuedElements = queues[queue].queuedElements;
		
		for(auto it = queuedElements.begin();it!= queuedElements.end();++it)
			if(*it == element)
				return false;
		
		ElementInfo ei;
		ei.age = 0;
		ei.score = score;
		elementsInfo[element] = ei;
		queuedElements.push_back(element);
		return true;
	}
	
	void QueueManager::removeElement(void* element)
	{
		if(elementsInfo.find(element) == elementsInfo.end())
			return;
		for(auto it = queues.begin(); it != queues.end();it++)
		{
			
			for(auto qit = it->second.queuedElements.begin(); qit != it->second.queuedElements.end(); ++qit)
			{
				if(element == *qit)
					qit = it->second.queuedElements.erase(qit);
			}
			
		}
		elementsInfo.erase(element);
		
		return ;
	}
	
	
	unsigned int QueueManager::queueSize(unsigned int queue)
	{
		if(queues.find(queue) == queues.end())
			return 0;
		return queues[queue].queuedElements.size();
		
	}
	
	std::list<void*>::iterator QueueManager::queueReadOldest(unsigned int id)
	{
		auto &queue = queues[id].queuedElements;
		auto current = queue.begin();
		auto best = current;
		auto best_age = elementsInfo[*current].age;
		
		while(current != queue.end())
		{
			auto age = elementsInfo[*current].age;
			if(age > best_age)
			{
				best = current;
				best_age = age;
			}
			++current;
		}
		return best;
	}
	
	
	
	
	std::list<void*>::iterator QueueManager::queueReadNearest(unsigned int id,double reference)
	{
		auto &queue = queues[id].queuedElements;
		auto current = queue.begin();
		auto best = current;
		double best_distance = std::abs(elementsInfo[*current].score-reference);
		
		while(current != queue.end())
		{
			double distance = std::abs(elementsInfo[*current].score-reference);
			if(distance < best_distance)
			{
				best = current;
				best_distance = distance;
			}
			++current;
		}
		return best;
	}
	
	
	
	QueueElement QueueManager::queueExtractBest(unsigned int id)
	{
		QueueElement qe;
		qe.queue = id;
		unsigned int group_size = queues[id].group_size;
		auto &queue = queues[id].queuedElements;
		
		auto oldest = queueReadOldest(id);
		if(queue.size() < group_size)
			return qe;
		if(oldest == queue.end())
			return qe;
			
		if(elementsInfo[*oldest].age < MAX_AGE)
			return qe;
		
		qe.elements.push_back(*oldest);
		removeElement(*oldest);
		unsigned int numero = 1;
		double average_score = elementsInfo[*oldest].score;
		while(numero < group_size)
		{
			auto altro = queueReadNearest(id,average_score);
			
			double nuovop = elementsInfo[*altro].score;
			average_score = ((average_score * numero) + nuovop) / (numero + 1.0);
			qe.elements.push_back(*altro);
			removeElement(*altro);
			numero++;
		}
		
		return qe;
		
	}
	
	
	
	std::vector<QueueElement> QueueManager::extractBest()
	{
		for(auto it = elementsInfo.begin(); it != elementsInfo.end(); ++it)
			it->second.age++;
		std::vector<QueueElement> ret;
		double factor = 0.3;
		
		std::vector<unsigned int> ids;
		for(auto it = queues.begin(); it != queues.end();it++)
			ids.push_back(it->first);
		std::random_shuffle (ids.begin(),ids.end());
		for(int i = 0; i < ids.size();i++)
		{
			
			unsigned int id = ids[i];
			unsigned int qs = queueSize(id);
			unsigned int gs = queues[id].group_size;
			if(qs < gs)
				continue;
			unsigned int expectedRemoved = (unsigned int ) (factor * qs);
			unsigned int wantedRemoved = std::max( gs, (expectedRemoved / qs) * qs );
			unsigned int expectedRemaining = qs - wantedRemoved;
			while(queueSize(id) >= gs && queueSize(id) > expectedRemaining )
			{
				QueueElement qe = queueExtractBest(id);
				if(qe.elements.size() == gs)
					ret.push_back(qe);
				else
					break;
				
			}
			
		}
		
		
		
		
		return ret;
	}
	
	
	
	
	
	
	
	
	
}
