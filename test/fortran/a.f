      program hello
      CHARACTER(len=32) :: arg
      CALL getarg(0, arg)
      print *, 'Hello World!', arg
      end program hello
