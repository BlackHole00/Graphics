package cyclic_cell

@(require) import "base:runtime"
@(require) import "core:sync"

Cyclic_Cell :: struct($T: typeid) {
	cells:		[3]Raw_Cell(T),
	consumer_cell:	int,
	producer_cell:	int,
}


create :: proc(
	cell: ^Cyclic_Cell($T),
	a0: runtime.Allocator = {},
	a1: runtime.Allocator = {},
	a2: runtime.Allocator = {},
) {
	cell.cells[0].allocator = a0
	cell.cells[1].allocator = a1
	cell.cells[2].allocator = a2

	cell.cells[0].state = .Producer_Writing
	cell.cells[1].state = .Ready_For_Producer
	cell.cells[2].state = .Consumer_Reading

	cell.producer_cell = 0
	cell.consumer_cell = 2
}

produce :: proc(cell: ^Cyclic_Cell($T)) {
	when ODIN_DEBUG do if sync.atomic_load(&cell.cells[cell.producer_cell].state) != State.Producer_Writing {
		panic("Illegal state")
	}

	next_cell := (cell.producer_cell + 1) % 3

	if _, is_next_cell_free := sync.atomic_compare_exchange_strong(
		&cell.cells[next_cell].state,
		State.Ready_For_Producer,
		State.Producer_Writing,
	); is_next_cell_free {
		sync.atomic_store(&cell.cells[cell.producer_cell].state, State.Ready_For_Consumer)
		cell.producer_cell = next_cell
	}

	cell.cells[cell.producer_cell].data = {}
	free_all(cell.cells[cell.producer_cell].allocator)
}

consume :: proc(cell: ^Cyclic_Cell($T)) {
	when ODIN_DEBUG do if sync.atomic_load(&cell.cells[cell.consumer_cell].state) != State.Consumer_Reading {
		panic("Illegal state")
	}

	next_cell := (cell.consumer_cell + 1) % 3

	if _, is_next_cell_free := sync.atomic_compare_exchange_strong(
		&cell.cells[next_cell].state,
		State.Ready_For_Consumer,
		State.Consumer_Reading
	); is_next_cell_free {
		sync.atomic_store(&cell.cells[cell.consumer_cell].state, State.Ready_For_Producer)
		cell.consumer_cell = next_cell
	}
}

producer_data :: proc(cell: ^Cyclic_Cell($T)) -> ^T {
	return &cell.cells[cell.producer_cell].data
}

consumer_data :: proc(cell: ^Cyclic_Cell($T)) -> ^T {
	return &cell.cells[cell.consumer_cell].data
}

allocator :: proc(cell: ^Cyclic_Cell($T)) -> runtime.Allocator {
	return cell.cells[cell.producer_cell].allocator
}

@(private)
State :: enum {
	Producer_Writing = 0,
	Ready_For_Consumer,
	Consumer_Reading,
	Ready_For_Producer,
}

@(private)
Raw_Cell :: struct($T: typeid) {
	state:		State,
	allocator:	runtime.Allocator,
	data:		T,
}

