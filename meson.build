project(
  'Arena.h',
  'c',
  meson_version : '>= 1.3.0',
  version : '0.1',
  default_options : ['c_std=gnu23', 'warning_level=3'],
)

home = include_directories('.')

sources = [
  'Arena.h',
  'unit_test/unit_test_assert.h',
  'unit_test/unit_test_log.h'
]

arena_pointer_test = executable(
  'POINTER TEST (TEST)',
  sources + ['test_cases/arena_pointer/arena_pointer_test.c'],
  install : false,
)

arena_layout_test = executable(
  'LAYOUT TEST (TEST)',
  sources + ['test_cases/arena_layout/arena_layout_test.c'],
  install : false,
)

arena_thread_test = executable(
  'ARENA THREAD TEST (TEST)',
  sources + ['test_cases/arena_thread/arena_thread_test.c'],
  install : false,
)

arena_stack_test = executable(
  'ARENA STACK TEST (TEST)',
  sources + ['test_cases/arena_stack/arena_stack_test.c'],
  install : false,
)

test('TEST POINTER', arena_pointer_test)
test('TEST LAYOUT', arena_layout_test)
test('TEST THREAD', arena_thread_test)
test('TEST STACK', arena_stack_test)

