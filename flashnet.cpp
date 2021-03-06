/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009,2010  Alessandro Pignotti (a.pignotti@sssup.it)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
**************************************************************************/

#include "abc.h"
#include "flashnet.h"
#include "class.h"
#include "flv.h"
#include "fastpaths.h"

using namespace std;
using namespace lightspark;

REGISTER_CLASS_NAME(URLLoader);
REGISTER_CLASS_NAME(URLLoaderDataFormat);
REGISTER_CLASS_NAME(URLRequest);
REGISTER_CLASS_NAME(URLVariables);
REGISTER_CLASS_NAME(SharedObject);
REGISTER_CLASS_NAME(ObjectEncoding);
REGISTER_CLASS_NAME(NetConnection);
REGISTER_CLASS_NAME(NetStream);

URLRequest::URLRequest()
{
}

void URLRequest::sinit(Class_base* c)
{
	c->setConstructor(new Function(_constructor));
}

void URLRequest::buildTraits(ASObject* o)
{
	o->setSetterByQName("url","",new Function(_setUrl));
	o->setGetterByQName("url","",new Function(_getUrl));
}

ASFUNCTIONBODY(URLRequest,_constructor)
{
	URLRequest* th=static_cast<URLRequest*>(obj);
	if(argslen>0 && args[0]->getObjectType()==T_STRING)
		th->url=args[0]->toString();
	return NULL;
}

ASFUNCTIONBODY(URLRequest,_setUrl)
{
	URLRequest* th=static_cast<URLRequest*>(obj);
	th->url=args[0]->toString();
	return NULL;
}

ASFUNCTIONBODY(URLRequest,_getUrl)
{
	URLRequest* th=static_cast<URLRequest*>(obj);
	return Class<ASString>::getInstanceS(th->url);
}

URLLoader::URLLoader():dataFormat("text"),data(NULL)
{
}

void URLLoader::sinit(Class_base* c)
{
	c->setConstructor(new Function(_constructor));
	c->super=Class<EventDispatcher>::getClass();
	c->max_level=c->super->max_level+1;
}

void URLLoader::buildTraits(ASObject* o)
{
	o->setGetterByQName("dataFormat","",new Function(_getDataFormat));
	o->setGetterByQName("data","",new Function(_getData));
	o->setSetterByQName("dataFormat","",new Function(_setDataFormat));
	o->setVariableByQName("load","",new Function(load));
}

ASFUNCTIONBODY(URLLoader,_constructor)
{
	EventDispatcher::_constructor(obj,NULL,0);
	return NULL;
}

ASFUNCTIONBODY(URLLoader,load)
{
	URLLoader* th=static_cast<URLLoader*>(obj);
	ASObject* arg=args[0];
	assert(arg->prototype==Class<URLRequest>::getClass());
	URLRequest* urlRequest=static_cast<URLRequest*>(arg);
	th->url=urlRequest->url;
	ASObject* data=arg->getVariableByQName("data","").obj;
	if(data)
	{
		if(data->prototype==Class<URLVariables>::getClass())
			::abort();
		else
		{
			const tiny_string& tmp=data->toString();
			//TODO: Url encode the string
			string tmp2;
			tmp2.reserve(tmp.len()*2);
			for(int i=0;i<tmp.len();i++)
			{
				if(tmp[i]==' ')
				{
					char buf[4];
					sprintf(buf,"%%%x",(unsigned char)tmp[i]);
					tmp2+=buf;
				}
				else
					tmp2.push_back(tmp[i]);
			}
			th->url+=tmp2.c_str();
		}
	}
	assert(th->dataFormat=="binary" || th->dataFormat=="text");
	sys->addJob(th);
	return NULL;
}

void URLLoader::execute()
{
	CurlDownloader curlDownloader(url);

	curlDownloader.run();
	if(!curlDownloader.hasFailed())
	{
		if(dataFormat=="binary")
		{
			ByteArray* byteArray=Class<ByteArray>::getInstanceS();
			byteArray->acquireBuffer(curlDownloader.getBuffer(),curlDownloader.getLen());
			data=byteArray;
		}
		else if(dataFormat=="text")
		{
			if(curlDownloader.getLen())
				threadAbort();
			data=Class<ASString>::getInstanceS();
		}
		//Send a complete event for this object
		sys->currentVm->addEvent(this,Class<Event>::getInstanceS("complete",this));
	}
	else
	{
		//Notify an error during loading
		sys->currentVm->addEvent(this,Class<Event>::getInstanceS("ioError",this));
	}
}

void URLLoader::threadAbort()
{
	//TODO: implement
	::abort();
}

ASFUNCTIONBODY(URLLoader,_getDataFormat)
{
	URLLoader* th=static_cast<URLLoader*>(obj);
	return Class<ASString>::getInstanceS(th->dataFormat);
}

ASFUNCTIONBODY(URLLoader,_getData)
{
	URLLoader* th=static_cast<URLLoader*>(obj);
	if(th->data==NULL)
		return new Undefined;
	
	th->data->incRef();
	return th->data;
}

ASFUNCTIONBODY(URLLoader,_setDataFormat)
{
	URLLoader* th=static_cast<URLLoader*>(obj);
	assert(args[0]);
	th->dataFormat=args[0]->toString();
	return NULL;
}

void URLLoaderDataFormat::sinit(Class_base* c)
{
	c->setVariableByQName("VARIABLES","",Class<ASString>::getInstanceS("variables"));
	c->setVariableByQName("TEXT","",Class<ASString>::getInstanceS("text"));
	c->setVariableByQName("BINARY","",Class<ASString>::getInstanceS("binary"));
}

void SharedObject::sinit(Class_base* c)
{
};

void ObjectEncoding::sinit(Class_base* c)
{
	c->setVariableByQName("AMF0","",new Integer(0));
	c->setVariableByQName("AMF3","",new Integer(3));
	c->setVariableByQName("DEFAULT","",new Integer(3));
};

NetConnection::NetConnection():isFMS(false)
{
}

void NetConnection::sinit(Class_base* c)
{
	//assert(c->constructor==NULL);
	//c->constructor=new Function(_constructor);
	c->super=Class<EventDispatcher>::getClass();
	c->max_level=c->super->max_level+1;
}

void NetConnection::buildTraits(ASObject* o)
{
	o->setVariableByQName("connect","",new Function(connect));
}

ASFUNCTIONBODY(NetConnection,connect)
{
	NetConnection* th=Class<NetConnection>::cast(obj);
	assert(argslen==1);
	if(args[0]->getObjectType()!=T_UNDEFINED)
	{
		th->isFMS=true;
		abort();
	}

	//When the URI is undefined the connect is successful (tested on Adobe player)
	Event* status=Class<NetStatusEvent>::getInstanceS("status", "NetConnection.Connect.Success");
	getVm()->addEvent(th, status);
	return NULL;
}

NetStream::NetStream():frameRate(0),downloader(NULL),decoder(NULL)
{
	sem_init(&mutex,0,1);
}

NetStream::~NetStream()
{
	assert(!executing);
	delete decoder; 
	sem_destroy(&mutex);
}

void NetStream::sinit(Class_base* c)
{
	c->setConstructor(new Function(_constructor));
	c->super=Class<EventDispatcher>::getClass();
	c->max_level=c->super->max_level+1;
}

void NetStream::buildTraits(ASObject* o)
{
	o->setVariableByQName("play","",new Function(play));
	o->setVariableByQName("close","",new Function(close));
	o->setGetterByQName("bytesLoaded","",new Function(getBytesLoaded));
	o->setGetterByQName("bytesTotal","",new Function(getBytesTotal));
	o->setGetterByQName("time","",new Function(getTime));
}

ASFUNCTIONBODY(NetStream,_constructor)
{
	LOG(LOG_CALLS,"NetStream constructor");
	assert(argslen==1);
	assert(args[0]->prototype==Class<NetConnection>::getClass());

	NetConnection* netConnection = Class<NetConnection>::cast(args[0]);
	assert(netConnection->isFMS==false);
	return NULL;
}

ASFUNCTIONBODY(NetStream,play)
{
	NetStream* th=Class<NetStream>::cast(obj);
	assert(argslen==1);
	const tiny_string& arg0=args[0]->toString();
	th->url = arg0;
	th->downloader=sys->downloadManager->download(th->url);
	sys->addJob(th);
	return NULL;
}

ASFUNCTIONBODY(NetStream,close)
{
	NetStream* th=Class<NetStream>::cast(obj);
	if(th->downloader)
		th->downloader->stop();
	th->threadAbort();
	return NULL;
}

NetStream::STREAM_TYPE NetStream::classifyStream(istream& s)
{
	char buf[3];
	s.read(buf,3);
	STREAM_TYPE ret;
	if(strncmp(buf,"FLV",3)==0)
		ret=FLV_STREAM;
	else
		threadAbort();

	s.seekg(0);
	return ret;
}

//Tick is called from the timer thread, this happens only if a decoder is available
void NetStream::tick()
{
	//Discard the first frame, if available
	//No mutex needed, ticking can happen only when decoder is valid
	if(decoder)
		decoder->discardFrame();
}

void NetStream::execute()
{
	//mutex access to downloader
	istream s(downloader);
	s.exceptions ( istream::eofbit | istream::failbit | istream::badbit );

	ThreadProfile* profile=sys->allocateProfiler(RGB(0,0,200));
	profile->setTag("NetStream");
	//We need to catch possible EOF and other error condition in the non reliable stream
	try
	{
		Chronometer chronometer;
		STREAM_TYPE t=classifyStream(s);
		if(t==FLV_STREAM)
		{
			FLV_HEADER h(s);
			if(!h.isValid())
				threadAbort();

			unsigned int prevSize=0;
			bool done=false;
			do
			{
				UI32 PreviousTagSize;
				s >> PreviousTagSize;
				PreviousTagSize.bswap();
				assert(PreviousTagSize==prevSize);

				//Check tag type and read it
				UI8 TagType;
				s >> TagType;
				switch(TagType)
				{
					case 8:
					{
						AudioDataTag tag(s);
						prevSize=tag.getTotalLen();
						break;
					}
					case 9:
					{
						VideoDataTag tag(s);
						prevSize=tag.getTotalLen();
						assert(tag.codecId==7);

						if(tag.isHeader())
						{
							//The tag is the header, initialize decoding
							assert(decoder==NULL); //The decoder can be set only once
							//NOTE: there is not need to mutex the decoder, as an async transition from NULL to
							//valid is not critical
							decoder=new FFMpegDecoder(tag.packetData,tag.packetLen);
							assert(decoder);
							assert(frameRate!=0);
							//Now that the decoder is valid, let's start the ticking
							sys->addTick(1000/frameRate,this);
							//sys->setRenderRate(frameRate);

							tag.releaseBuffer();
						}
						else
							decoder->decodeData(tag.packetData,tag.packetLen);
						break;
					}
					case 18:
					{
						ScriptDataTag tag(s);
						prevSize=tag.getTotalLen();

						//HACK: initialize frameRate from the container
						frameRate=tag.frameRate;
						break;
					}
					default:
						cout << (int)TagType << endl;
						threadAbort();
				}
				profile->accountTime(chronometer.checkpoint());
				if(aborting)
					throw JobTerminationException();
			}
			while(!done);
		}
		else
			threadAbort();

	}
	catch(LightsparkException& e)
	{
		cout << e.what() << endl;
		threadAbort();
	}
	catch(JobTerminationException& e)
	{
	}

	sem_wait(&mutex);
	sys->downloadManager->destroy(downloader);
	downloader=NULL;
	//This transition is critical, so the mutex is needed
	//Before deleting stops ticking, removeJobs also spin waits for termination
	sys->removeJob(this);
	delete decoder;
	decoder=NULL;
	sem_post(&mutex);
/*	Event* status=Class<NetStatusEvent>::getInstanceS(true, "status", "NetStream.Play.Start");
	getVm()->addEvent(this, status);
	status=Class<NetStatusEvent>::getInstanceS(true, "status", "NetStream.Buffer.Full");
	getVm()->addEvent(this, status);*/
}

void NetStream::threadAbort()
{
	sem_wait(&mutex);
	if(downloader)
		downloader->stop();
	//Discard a frame, the decoder may be blocked on a full buffer
	if(decoder)
		decoder->discardFrame();
	sem_post(&mutex);
}

ASFUNCTIONBODY(NetStream,getBytesLoaded)
{
	return abstract_i(0);
}

ASFUNCTIONBODY(NetStream,getBytesTotal)
{
	return abstract_i(100);
}

ASFUNCTIONBODY(NetStream,getTime)
{
	return abstract_d(0);
}

uint32_t NetStream::getVideoWidth()
{
	sem_wait(&mutex);
	uint32_t ret=0;
	if(decoder)
		ret=decoder->getWidth();
	sem_post(&mutex);
	return ret;
}

uint32_t NetStream::getVideoHeight()
{
	sem_wait(&mutex);
	uint32_t ret=0;
	if(decoder)
		ret=decoder->getHeight();
	sem_post(&mutex);
	return ret;
}

double NetStream::getFrameRate()
{
	return frameRate;
}

bool NetStream::copyFrameToTexture(GLuint tex)
{
	sem_wait(&mutex);
	bool ret=false;
	if(decoder)
		ret=decoder->copyFrameToTexture(tex);
	sem_post(&mutex);
	return ret;
}

void URLVariables::sinit(Class_base* c)
{
	c->setConstructor(new Function(_constructor));
}

ASFUNCTIONBODY(URLVariables,_constructor)
{
	assert(argslen==0);
	return NULL;
}

tiny_string URLVariables::toString(bool debugMsg)
{
	assert(implEnable);
	//Should urlencode
	::abort();
	if(debugMsg)
		return ASObject::toString(debugMsg);
	int size=numVariables();
	tiny_string ret;
	for(int i=0;i<size;i++)
	{
		const tiny_string& tmp=getNameAt(i);
		if(tmp=="")
			abort();
		ret+=tmp;
		ret+="=";
		ret+=getValueAt(i)->toString();
		if(i!=size-1)
			ret+="&";
	}
	return ret;
}

