
tx_init() {
	ïnt ws_counter;

	thread *t = malloc(sizeof(thread_d));
	memset(t, 0, sizeof(thread_d);

	//todo: maybe need unique ID, can use fetch&add
	t->local_version = 0;
	t->writer_version = MAX_VERSION;


	
}

tx_finish() {
	//free allocated memory
}

tx_reader_lock(shared_t* shared, thread *t) {
	t->n_starts++;
	//not atomic enough probably have to use clocks and check for steals
	//as they are monotoniclly increasing, should problly mostly avoid boolean locks
	if (read_copy) {
		shared->readers_in_cpy++; 
	}
}

tx_reader_unlock(shared_t* shared) {
	t->n_starts++;

}

//returns the pointer to the start of region to be modified
void* tx_deref(thread *t, shared_t shared) {
	
	if (shared.is_copy) {
		return shared.start;
	}

	if (shared.copy_clock <= t->l_clock) {
		return shared.start_cpy;
	}
	return shared.start;

}


//return a poitner to a copy of the shared memory that is safe to modify
tx_try_write_lock(shared_t* shared) {
	if (shared->w_lock) { //TODO: use CAS to lock object
		return false; //wait until current write finishes
	}

	shared->w_lock = true;
	memcpy(shared->start, shared->start_cpy, shared->size);
	return true;


}

tx_synch(shared_t* shared) {
	shared->read_copy = true;

	while (readers_in_obj != 0) {/*wait*/ } //maybe use a semaphore or somthing here

}

tx_commit_write_log(shared_t* shared) {
	//TODO: advance global clock if neeeded
	//shared->g_clock++;F&I(g_clock);...

	tx_synch(shared);
	memcpy(shared->start_cpy, shared->start, size);
	shared->w_lock = false;

}

