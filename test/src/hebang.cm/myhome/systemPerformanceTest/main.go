package main

import "C"
import (
	"bytes"
	"encoding/xml"
	"fmt"
	"log"
	"os"
	"path/filepath"
	"syscall"
	"unsafe"
)

func main() {
	log.Println("start...")

	dllPath, err := getDllPath()
	if err != nil {
		fmt.Printf("fail to get dll path, err=%s\n", err)
		return
	}

	libHandle, err := syscall.LoadLibrary(dllPath) //必须是64位的dll
	if err != nil {
		fmt.Printf("load dll fail err=%s\n", err)
		return
	}
	defer syscall.FreeLibrary(libHandle) //记得要释放dll

	//获取dll中的函数的地址
	initFun, err := syscall.GetProcAddress(libHandle, "hebPerformanceInit")
	if err != nil {
		log.Printf("fail to get 'hebPerformanceInit' fun addr, err=%s\n", err)
		return
	}

	statusFun, err := syscall.GetProcAddress(libHandle, "hebGetPerformance")
	if err != nil {
		log.Printf("fail to get 'hebGetPerformance' fun addr, err=%s\n", err)
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
		//fmt.Printf("%s\n", jsonStr)

		data := allAdapterStatus{}
		err = xmlStr2Obj(&jsonStr, &data)
		if nil != err {
			log.Printf("fail to parse adapter json string, err=%s\n", err)
			return
		}

		for _, adapter := range data.Adapters {
			fmt.Printf("adapter[%s, %s, %d] recvSpeed=%s, sendSpeed=%s\n", adapter.Name, adapter.IPs, adapter.MaxSpeedBit, adapter.RecvSpeed.Str, adapter.Sendpeed.Str)
		}
		fmt.Println("")
	}
}

func getDllPath() (dllPath string, retErr error) {
	dllName := "getSystemPerformance.dll"

	exeDir, err := filepath.Abs(filepath.Dir(os.Args[0]))
	if err != nil {
		retErr = fmt.Errorf("fail to get exe dir, err=%s", err)
		return
	}

	dllPath = exeDir + "/" + dllName
	if _, err := os.Stat(dllPath); nil == err {
		return
	}

	dllPath, err = filepath.Abs(exeDir + "/../../../../../")
	if err != nil {
		retErr = fmt.Errorf("fail to get top dir, err=%s", err)
		return
	}

	dllPath += "/out/x64/" + dllName
	if _, err := os.Stat(dllPath); err != nil {
		retErr = fmt.Errorf("fail to get dll path based top dir, err=%s", err)
		return
	}

	return
}

func xmlStr2Obj(str *string, obj interface{}) (err error) {
	reader := bytes.NewReader([]byte(*str))
	decoder := xml.NewDecoder(reader)
	err = decoder.Decode(obj)
	return
}

type allAdapterStatus struct {
	XMLName  xml.Name      `xml:"root"`
	Adapters []adapterInfo `xml:"adapter"`
}

type adapterInfo struct {
	Name        string         `xml:"name"`
	IPs         string         `xml:"ip"`
	MaxSpeedBit int64          `xml:"maxSpeedBit"`
	RecvSpeed   directionSpeed `xml:"recvSpeed"`
	Sendpeed    directionSpeed `xml:"sendSpeed"`
}

type directionSpeed struct {
	Bit int64  `xml:"bit"`
	Str string `xml:"str"`
}
