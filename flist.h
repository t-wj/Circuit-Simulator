#ifndef FLIST_H
#define FLIST_H
#include <assert.h>

template <class T>
class fList
{
private:
	class node
	{
	public:
		node(const T &_data,node *_next):data(_data),next(_next){}
		T data;//数据
		node *next;//指向下一节点的指针
	};
	node *head;//链表头

public:
	class iterator
	{
	public:
		friend class fList;
		iterator &operator++() {assert(nodePtr!=NULL); nodePtr=nodePtr->next; return *this;}
		T &operator*() {return nodePtr->data;}
		bool operator!=(const iterator &it1) const {return (nodePtr!=it1.nodePtr)?true:false;}
		bool operator==(const iterator &it1) const {return (nodePtr==it1.nodePtr)?true:false;}
		iterator(node *_nodePtr=NULL):nodePtr(_nodePtr){}
	private:
		node *nodePtr;//迭代器指向的节点
	};
	T &front() {return head->data;}
	iterator begin() {return iterator(head);}
	iterator end() {return iterator(NULL);}
	iterator cbegin() const {return iterator(head);}
	iterator cend() const {return iterator(NULL);}
	void pop_front() {assert(!empty()); node *tmp=head->next; delete head; head=tmp;}
	void push_front(const T &val) {head=new node(val,head);}
	bool empty() const {return (cbegin()==cend())?true:false;}
	void reverse();
	void remove(const T &val);
	iterator erase_after(const iterator &it);
	iterator insert_after(const iterator &it,const T &val);
	void clear();
	fList():head(NULL){}
	fList(const fList &src):head(NULL)
	{//拷贝构造函数
		fList<T> tmp;
		for(iterator it=src.cbegin();it!=src.cend();++it)
			tmp.push_front(*it);
		for(iterator it=tmp.begin();it!=tmp.end();++it)
			push_front(*it);
	}
	~fList() {clear();}
};

template <class T>
void fList<T>::reverse()
{//将链表反转
    if(!empty()&&++begin()!=end())
    {//若链表元素个数大于1，则继续操作
        node *p=head,*q=head->next,*r;
        p->next=NULL;//将原表头变成表尾
        while(q)
        {
            r=q->next;//存储当前q的后继
            q->next=p;//将原前驱与后继交换，即将后继指向下一节点的指针指向前驱
            p=q;//将p、q指针向前移
            q=r;
        }
        head=p;//新表头即为原表尾
    }
}

template <class T>
void fList<T>::remove(const T &val)
{//移除所有值为val的节点
	iterator it=begin();
	while(!empty()&&*it==val)
	{
		head=it.nodePtr->next;
		delete it.nodePtr;
		it=begin();
	}
	if(!empty())
	{
		++it;
		for(iterator before_it=begin(),after_it;it!=end();)
			if(*it==val)
			{
				after_it=it;
				++after_it;
				delete it.nodePtr;
				before_it.nodePtr->next=after_it.nodePtr;
				it=after_it;
			}
			else
			{
				before_it=it;
				++it;
			}
	}
}

template <class T>
typename fList<T>::iterator fList<T>::erase_after(const iterator &it)
{//删除it后一个节点
	iterator tmp=it;
	assert(tmp!=end()&&++tmp!=end());
	it.nodePtr->next=tmp.nodePtr->next;
	delete tmp.nodePtr;
	return iterator(it.nodePtr->next);
}

template <class T>
typename fList<T>::iterator fList<T>::insert_after(const iterator &it,const T &val)
{//在it后添加一个节点
	assert(it!=end());
	return iterator(it.nodePtr->next=new node(val,it.nodePtr->next));
}

template <class T>
void fList<T>::clear()
{//清除所有节点
	if(!empty())
	{
		iterator before_it=begin(),it=begin();
		for(++it;it!=end();++it)
		{
			delete before_it.nodePtr;
			before_it=it;
		}
		delete before_it.nodePtr;
	}
	head=NULL;
}

#endif // FLIST_H
