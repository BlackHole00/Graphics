package main

import hm "core:container/handle_map"

G_Actor_Iterator :: struct {
	iterator: hm.Dynamic_Handle_Map_Iterator(hm.Dynamic_Handle_Map(G_Actor, G_Actor_Handle)),
}

G_Actor_Storage :: struct {
	actors:		hm.Dynamic_Handle_Map(G_Actor, G_Actor_Handle),
	removed_actors:	[dynamic]G_Actor_Handle,

	default_actor:	G_Actor,
}
g_actor_storage: G_Actor_Storage


g_init_actor_storage :: proc() {
	hm.dynamic_init(&g_actor_storage.actors, context.allocator)
	g_actor_storage.removed_actors = make([dynamic]G_Actor_Handle, context.allocator)

	g_actor_storage.default_actor.scale = { 1, 1, 1 }
}

g_fini_actor_storage :: proc() {
	hm.dynamic_destroy(&g_actor_storage.actors)
	delete(g_actor_storage.removed_actors)
}

g_add_actor :: proc(init_data: ^G_Actor = nil) -> (G_Actor_Handle, bool) #optional_ok {
	actor_data: G_Actor
	if init_data != nil {
		actor_data = init_data^
	}
	
	handle, err := hm.add(&g_actor_storage.actors, actor_data)
	if err != .None {
		return {}, false
	}

	return handle, true
}

g_actor_count :: proc() -> uint {
	return hm.len(g_actor_storage.actors)
}

g_get_actor :: proc(handle: G_Actor_Handle) -> (^G_Actor, bool) #optional_ok {
	actor, ok := hm.get(&g_actor_storage.actors, handle)

	if !ok {
		return &g_actor_storage.default_actor, false
	} else {
		return actor, ok
	}
}

g_queue_remove_actor :: proc(handle: G_Actor_Handle) -> bool {
	if g_is_actor_valid(handle) {
		return false
	}

	append(&g_actor_storage.removed_actors, handle)
	return true
}

g_is_actor_valid :: proc(handle: G_Actor_Handle) -> bool {
	return hm.is_valid(&g_actor_storage.actors, handle)
}

g_apply_queued_actor_removals :: proc() {
	for handle in g_actor_storage.removed_actors {
		hm.remove(&g_actor_storage.actors, handle)
	}

	clear(&g_actor_storage.removed_actors)
}

g_tick_actors :: proc() {
	iter := hm.iterator_make(&g_actor_storage.actors)
	for actor, _ in hm.iterate(&iter) {
		g_tick_actor(actor)
	}

	g_apply_queued_actor_removals()
}

g_actor_iterator :: proc() -> G_Actor_Iterator {
	return {
		hm.iterator_make(&g_actor_storage.actors)
	}
}

g_iterate_actors :: proc(iter: ^G_Actor_Iterator) -> (^G_Actor, G_Actor_Handle, bool) {
	return hm.dynamic_iterate(&iter.iterator)
}

