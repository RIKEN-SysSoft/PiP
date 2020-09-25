      program hello
      CHARACTER(len=32) :: arg
      CALL getarg(0, arg)
      print *, 'Hello World!'
      print *, arg
      end program hello
