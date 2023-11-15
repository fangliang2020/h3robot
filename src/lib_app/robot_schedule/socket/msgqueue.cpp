#include "msgqueue.h"

/**
 * @brief 初始化一个空队列,空队列的判定标准:首/尾节点重合(front==NULL 且 rear==NULL)
 * @return pointQueue 返回1个节点的队列
 */
void init_queue(MSGQUEUE* que)
{	
//	LOG_INFO("init_queue start:%s",que->queue_name);
    que->front = que->rear =NULL;
//    LOG_INFO("init_queue end:%s",que->queue_name);
}

int queue_empty(MSGQUEUE* que)
{
	if(que!=NULL){
		if (que->front == NULL && que->rear==NULL){
			return 1;
		}
	}    
    return 0;
}

/**
 * @brief 入队,入队之后需要重置队列
 * @param que   全局队列
 * @param msg  新消息
 * @note enqueue函数涉及到多个线程入队,所有这里加锁处理,防止不同线程调用冲突
 */
void enqueue(MSGQUEUE* que, MSG_STRUCT msg)
{
	pthread_mutex_lock(&g_enqueue_lock);
	if(msg.type==0){
		printf("enqueue_start:%s:%d",que->queue_name,msg.type);
	}
    QUEUE_NODE *newNnode = (QUEUE_NODE *)malloc(sizeof(QUEUE_NODE));
    if(newNnode==NULL)
    {
		printf("newNnode malloc fail\n");
    }
    else
    {
        newNnode->node=msg;	//将msg放入新生成的节点
        newNnode->next = NULL;
		if(que->front ==NULL && que->rear ==NULL){	//初始为空节点的时候,front=rear=newNode
			que->front = que->rear = newNnode;		//生成一个节点的队列	
		}else{ //初始为非空节点,直接将新节点放rear
			QUEUE_NODE* rearold = que->rear;			//取尾部节点,以便指向新的rear
			que->rear = newNnode;		//rear指向新节点(重置rear)	
			rearold->next = que->rear;
		}		
		que->rear->next = NULL;
    }
	if(msg.type==0){
		printf("enqueue_ennd:%s:%d",que->queue_name,msg.type);
	}	
	print_queue(que);
	pthread_mutex_unlock(&g_enqueue_lock);
}

/**
 * @brief 出队,出队之后需要重置队列
 * @param que   输入队列
 * @return MSG_STRUCT 头部节点
 */
MSG_STRUCT dequeue(MSGQUEUE* que)
{	
	pthread_mutex_lock(&g_dequeue_lock);
//	LOG_INFO("dequeue_start:%s",que->queue_name);
	QUEUE_NODE out;out.node.type=NULL_TYPE;
	if(!queue_empty(que)){
		if(que->front == que->rear){ //只有1个节点	
//			LOG_INFO("only one node");
			out = *(que->front);
			que->front =que->rear=NULL; //置空
			print_queue(que);
		}else{  //多个节点
			out = *(que->front);	 //保存第1个节点
			QUEUE_NODE *second = que->front->next; //保存第2个节点
			if(que->front == que->rear){ //只有2个节点的时候,取走1个只剩1个
//				LOG_INFO("have two nodes");
				que->front = que->rear;
//				print_queue(que);
			}else{	//2个以上的节点
				que->front =second;
				que->front->next = second->next;	
//				LOG_INFO("have more node");
//				print_queue(que);
			}									
		}
//		LOG_INFO("dequeue ennd:%s",que->queue_name);
	}else{
//		LOG_INFO("empty");
	}
	pthread_mutex_unlock(&g_dequeue_lock);
	if(out.node.type==0){
		printf("dddddqueue:%d",out.node.type);
	}
//	else{
//		LOG_INFO("xxxxxqueue:%d",out.node.type);
//	}
    return out.node;
}

void print_queue(MSGQUEUE* que){
/**
	if(!queue_empty(que)){
		QUEUE_NODE *head =que->front;
		while(1){			
			if(head!=NULL){
				LOG_INFO("msg_id:%d,type:%d",head->node.id,head->node.type);
				head = head->next;
			}else{
				break;
			}
		}
	}else{
		LOG_INFO("que empty");
	}
**/	
}




