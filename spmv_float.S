	.text
	.align 2
	.globl spmv_float
    .type spmv_float, @function
	
spmv_float:
	fsub.s 		$f0,	$f0,	$f0
	srli.d		$t1,	$a0,	3
	andi		$t0,	$a0,	0x07

.LOOP_MULX:
	beq			$t1, 	$zero, 	.REST_MULX
	xvld		$xr1,   $a1,    0
	addi.d		$a1, 	$a1, 	32
	xvld        $xr2,   $a2,    0
	addi.d		$a2, 	$a2, 	32
    b           .LOAD_VEC
.RET_LOOP:
	xvfmul.s   	$xr4,   $xr1,   $xr3
	xvpermi.q	$xr7,	$xr4,	1
	vfadd.s		$vr8,	$vr7,	$vr4
	vpermi.w	$vr7,	$vr8,	0xe
	vfadd.s 	$vr4,	$vr7,	$vr8
	xvpickve.w 	$xr5,   $xr4,   1
	fadd.s 		$f0,	$f0,	$f5
	fadd.s 		$f0,	$f0,	$f4
	addi.d      $t1,     $t1,    -1
	b           .LOOP_MULX
		
.REST_MULX:
	beq			$t0, 	$zero, 	.END
	fld.s       $f1,    $a1,    0   
	addi.d      $a1,    $a1,    4
	ld.wu		$t2,	$a2,	0   
	addi.d		$a2, 	$a2, 	4
	slli.w      $t2,	$t2,    2   
	fldx.s 		$f6,    $a3,	$t2 
	fmul.s      $f7,    $f6,    $f1 
	fadd.s      $f0,    $f0,    $f7
	addi.d		$t0,	$t0,	-1
	b           .REST_MULX

.END:
	jr $ra
	
.LOAD_VEC:
	xvslli.w 			$xr2,	$xr2,	2
	xvpickve2gr.wu 		$t2,    $xr2,   0  
    fldx.s 		        $f6,    $a3,	$t2 
	xvinsve0.w          $xr3,   $xr6,   0  

	xvpickve2gr.wu 		$t2,    $xr2,   1  
    fldx.s 		        $f6,    $a3,	$t2 
	xvinsve0.w          $xr3,   $xr6,   1  
	
	xvpickve2gr.wu 		$t2,    $xr2,   2  
    fldx.s 		        $f6,    $a3,	$t2 
	xvinsve0.w          $xr3,   $xr6,   2  

	xvpickve2gr.wu 		$t2,    $xr2,   3  
    fldx.s 		        $f6,    $a3,	$t2 
	xvinsve0.w          $xr3,   $xr6,   3  

	xvpickve2gr.wu 		$t2,    $xr2,   4  
    fldx.s 		        $f6,    $a3,	$t2 
	xvinsve0.w          $xr3,   $xr6,   4  

	xvpickve2gr.wu 		$t2,    $xr2,   5  
    fldx.s 		        $f6,    $a3,	$t2 
	xvinsve0.w          $xr3,   $xr6,   5  

	xvpickve2gr.wu 		$t2,    $xr2,   6  
    fldx.s 		        $f6,    $a3,	$t2 
	xvinsve0.w          $xr3,   $xr6,   6  

	xvpickve2gr.wu 		$t2,    $xr2,   7  
    fldx.s 		        $f6,    $a3,	$t2 
	xvinsve0.w          $xr3,   $xr6,   7  

	b  .RET_LOOP
