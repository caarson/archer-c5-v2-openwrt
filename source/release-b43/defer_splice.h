/* Source lock protects concurrent enqueue; destination is private and empty.
 * No allocation, register access, callbacks, or payload access under this lock.
 * IRQ free/synchronize precedes queue purge and waits for the local consumer.
 */
static inline unsigned int b43_defer_take(struct sk_buff_head *source,
                                         struct sk_buff_head *local)
{
 unsigned long flags;
 unsigned int count;
 spin_lock_irqsave(&source->lock, flags);
 count = skb_queue_len(source);
 skb_queue_splice_init(source, local);
 spin_unlock_irqrestore(&source->lock, flags);
 return count;
}
