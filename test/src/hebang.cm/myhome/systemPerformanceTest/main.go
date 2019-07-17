package main

import "C"
import (
	"fmt"
	"log"
	"syscall"
	"unsafe"
)

func main() {
	log.Println("start...")

	libHandle, err := syscall.LoadLibrary("getSystemPerformance.dll") //必须是64位的dll
	if err != nil {
		log.Printf("load dll fail err=%s", err)
		return
	}
	defer syscall.FreeLibrary(libHandle) //记得要释放dll

	//获取dll中的函数的地址
	initFun, err := syscall.GetProcAddress(libHandle, "hebPerformanceInit")
	if err != nil {
		log.Printf("fail to get 'hebPerformanceInit' fun addr, err=%s", err)
		return
	}

	statusFun, err := syscall.GetProcAddress(libHandle, "hebGetPerformance")
	if err != nil {
		log.Printf("fail to get 'hebGetPerformance' fun addr, err=%s", err)
		return
	}

	//初始化dll
	retDll, _, _ := syscall.Syscall(uintptr(initFun), 0, 0, 0, 0)
	if 0 != retDll {
		log.Printf("fail to init 'getSystemPerformance.dll', ret=%d\n", retDll)
		return
	}

	//获取操作系统性能
	sliceCap := int32(10240)
	outSlice := make([]byte, sliceCap)
	out := uintptr((unsafe.Pointer(&(outSlice[0]))))

	retDll, _, _ = syscall.Syscall(uintptr(statusFun), 3, 1, out, uintptr(sliceCap))
	if retDll <= 0 {
		log.Printf("fail to init 'getSystemPerformance.dll', ret=%d\n", retDll)
		return
	}

	strLen := int(retDll)
	jsonStr := string(outSlice[:strLen])
	fmt.Printf("%s", jsonStr)
}
