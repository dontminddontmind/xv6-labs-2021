## 解题思路
### 数据结构
用数组保存每个页的cow引用次数，没有复制者就==0
```c
struct {
  int count[(PHYSTOP - KERNBASE) / PGSIZE]; // cow page reference count
  struct spinlock lock;
} pageref;
void pagerefadd(uint64 pa, int add); 
```
PTE_F记录是否是writable的PTE页
```c
#define PTE_F (1L << 8) // distinguish cow from non-writable page
```
> cow==0
>> PTE_F 单独的cow页，写入时直接取消掉PTE_F,并置位PTE_W，free时直接free页
>> !PTE_F 不可读的cow页或非cow页，free时直接free页
> cow!=0
>> PTE_F cow页，写入时复制页，取消掉PTE_F,并置位PTE_W，cow--，free时cow--
>> !PTE_F 不可读的cow页，free时cow--

### 函数
* uvmcopy cow++ 
mappage
if(writable)
 both unset(PTE_W) set(PTE_F)
{cow++}锁

* usertrap cow-- 
if(write && PTE_F)
  if(cow==0) unset(PTE_F) set(PTE_W)
  else {copypage cow--}锁 unset(PTE_F) set(PTE_W)
else panic

* copyout cow-- 
同上

* kfree cow--
{
如果cow==0就free
否则cow--
}锁

### 并发
cow==1
copypage
并发 free cow--
cow-- 负数了！
