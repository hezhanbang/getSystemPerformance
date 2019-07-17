package main

import "C"
import (
	"encoding/json"
	"encoding/xml"
	"fmt"
	"log"
	"syscall"
	"time"
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

	for {
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

		data := allAdapterStatus{}
		err = jsonStr2Obj(&jsonStr, &data)
		if nil != err {
			log.Printf("fail to parse adapter json string, err=%s\n", err)
			return
		}

		for _, adapter := range data.Adapters {
			fmt.Printf("adapter[%s, %d] recvSpeed=%s, sendSpeed=%s\n", adapter.Name, adapter.MaxSpeedBit, adapter.RecvSpeed.Str, adapter.Sendpeed.Str)
		}
		fmt.Println("")

		time.Sleep(time.Second * 2)
	}
}

func jsonStr2Obj(str *string, c interface{}) (err error) {
	err = json.Unmarshal([]byte(*str), c)
	return
}

type allAdapterStatus struct {
	XMLName  xml.Name      `xml:"root"`
	Adapters []adapterInfo `xml:"adapter"`
}

type adapterInfo struct {
	Name        string         `xml:"name"`
	MaxSpeedBit int            `xml:"maxSpeedBit"`
	RecvSpeed   directionSpeed `xml:"recvSpeed"`
	Sendpeed    directionSpeed `xml:"sendSpeed"`
}

type directionSpeed struct {
	Bit int64  `xml:"bit"`
	Str string `xml:"str"`
}
