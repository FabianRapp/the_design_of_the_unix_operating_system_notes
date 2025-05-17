/*
 * Page 59:
 * If several processes conted for a buffer, the kernel guarantees that none of
 * them sleep forever, but it does not guarantee that a process will not be
 * starved out from use of a buffer.
 * Redesign getblk so that a process is guaranteed eventual use of a buffer.
 */

// original (Page 44)
getblk
input:
	file system number
	block number
output:
	locked buffer that can now be used for block
{
	while (buffer not found)
	{
		if (block in hash queue)
		{
			if (buffer busy)
			{
				sleep(event buffer becomes free);
				continue ;
			}
			mark buffer busy;
			remove buffer from free list;
			return buffer;
		}
		else
		{
			if (there are no buffers on free list)
			{
				sleep(event any buffer becomes free);
				continue ;
			}
			remove buffer from old hash queeu;
			put buffer onto new hash queue;
			return buffer;
		}
	}
}

Page 46
algorithm brelse
input:
	locked buffer
output:
	none
{
	wakup all procs: event, waiting for any buffer to become free;
	wakup all procs: event, waiting for this buffer to become free;
	raise processor execution level to block interrupts;
	if (buffer contents valid and buffer not old)
		enqueue buffer at end of free list
	else
		enqueue buffer at beginning of free list
	lower processor execution level to allow interrupts;
	unlock(buffer);
}



/*
 * my altered versions
 */

getblk with guarantee
input:
	file system number
	block number
output:
	locked buffer that can now be used for block
{
	while (buffer not found)
	{
		if (block in hash queue)
		{
			// added explicit interrupt disabling for clearness
			// the version above would also need some, tho less aggressive
			raise cpu lvl to block interrupts;
			if (buffer busy || buffer->request_queue.size)
			{
				buffer->enque_request(process->pid); // <-- add ability to mark interest for a buffer so it does not get moved to free list
				lower cpu lvl to allow interrups;
				sleep(it is this process turn to use the buffer);
			}
			// make waiting process see that there was an error with the device they wait for and
			// not sure what to do here
			if (buffer content is not valid && buffer for read)
			{
				lower cpu lvl to allow interrups;
				continue ;
			}
			raise cpu lvl to block interrupts;
			mark buffer busy;
			buffer request queue pop;
			remove buffer from free list;
			lower cpu lvl to allow interrups;
			return buffer;
		}
		else
		{
			raise cpu lvl to block interrupts;
			cur_buf = free_list.beginning;
			while (cur_buf && cur_buf->request_queue.size)
			{
				cur_buf = cur_buf->next;
			}
			if (!cur_buf)
			{
				free_list.enque_request(process->pid);
				lower cpu lvl to allow interrups;
				sleep(it is this proceses turn to get a buffer from the free_list);
				raise cpu lvl to block interrupts;
			}

			remove cur_buf from old hash queeu;
			put cur_buf onto new hash queue;
			remove cur_buf from free_list;
			lower cpu lvl to allow interrups;
			return cur_buf;
		}
	}
}

algorithm brelse_modified
input:
	locked buffer
output:
	none
{
	if (buffer->request_queue.size)
	{
		buffer->busy = 0;
		wakeup buffer->request_queue.top;
		buffer->request_queue.pop();
	}
	else if (free_list.request_queue.size)
	{
		buffer->busy = 0;
		wakeup free_list.request_queue.top gets a buffer from the free list;
		free_list.request_queue.pop();
	}
	else
	{
		wakup all procs: event, waiting for any buffer to become free;
		wakup all procs: event, waiting for this buffer to become free;
		raise processor execution level to block interrupts;
		if (buffer contents valid and buffer not old)
			enqueue buffer at end of free list
		else
			enqueue buffer at beginning of free list
		lower processor execution level to allow interrupts;
	}
	unlock(buffer);
}
