threads = dependency('threads')

hydra_test = executable('hydra-test', 'test.cpp',
    dependencies: [hydra, threads])

test('all', hydra_test)

