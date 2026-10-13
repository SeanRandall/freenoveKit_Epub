#include "transfer_web.h"

/* The browser does the presentation work and talks to the existing WebDAV
   endpoint.  Keeping this page self-contained avoids another network service,
   filesystem assets, and any dependency on Internet access. */
static const char page[] =
"<!doctype html><html lang=en><head><meta charset=utf-8>"
"<meta name=viewport content='width=device-width,initial-scale=1'>"
"<title>Retroreader SD card</title><style>"
":root{color-scheme:light dark;font:100% system-ui,sans-serif}"
"body{max-width:60rem;margin:auto;padding:1rem;line-height:1.5}"
"button,input{font:inherit;padding:.45rem}.toolbar{display:flex;gap:.6rem;flex-wrap:wrap;align-items:center}"
"table{width:100%;border-collapse:collapse;margin-top:1rem}th,td{padding:.55rem;text-align:left;border-bottom:1px solid #888}"
"th:last-child,td:last-child{text-align:right}.drop{border:2px dashed #888;padding:1rem;margin:1rem 0}"
".drop.over{border-color:#0878d1;background:#0878d122}.muted{opacity:.75}"
"dialog{max-width:30rem;width:calc(100% - 3rem);padding:1.25rem}dialog::backdrop{background:#0008}"
"dialog .actions{display:flex;gap:.6rem;flex-wrap:wrap;margin:1rem 0}dialog label{display:block;margin:1rem 0}"
"@media(max-width:35rem){th:nth-child(2),td:nth-child(2){display:none}}"
"</style></head><body><main><h1>Retroreader SD card</h1>"
"<p id=location>Folder: /</p><div class=toolbar>"
"<button id=up type=button>Up one folder</button><button id=refresh type=button>Refresh</button>"
"<button id=newFolder type=button>New folder</button>"
"<label>Upload files <input id=files type=file multiple></label></div>"
"<div id=drop class=drop tabindex=0><strong>Drop files here</strong> or paste files copied from Explorer. "
"<span class=muted>Pasting depends on browser support.</span></div>"
"<p id=status role=status aria-live=polite></p><div id=progressBox hidden>"
"<label for=progress>Upload progress</label> <progress id=progress max=100></progress></div>"
"<table><thead><tr><th scope=col>Name</th><th scope=col>Size</th></tr></thead>"
"<tbody id=items></tbody></table></main>"
"<dialog id=itemDialog aria-labelledby=dialogTitle><form method=dialog><button>Close</button></form>"
"<h2 id=dialogTitle></h2><p id=itemDetails></p><div class=actions>"
"<button id=openItem type=button></button><button id=deleteItem type=button>Delete</button></div>"
"<label for=newName>New name or relative path</label><input id=newName>"
"<p class=muted>Include a folder path to move the item. Missing folders will be created.</p>"
"<button id=renameItem type=button>Rename or move</button>"
"</dialog><script>"
"let path='/',selected=null;const $=x=>document.getElementById(x),status=$('status'),progress=$('progress');"
"const part=s=>encodeURIComponent(s).replace(/%2F/gi,'/');"
"const url=(p=path)=>p.split('/').map(part).join('/');"
"function nice(n){if(n===null)return '';for(const u of ['B','KB','MB','GB']){if(n<1024||u==='GB')return n.toFixed(u==='B'?0:1)+' '+u;n/=1024}}"
"async function entries(p){const r=await fetch(url(p),{method:'PROPFIND',headers:{Depth:'1'}});if(!r.ok&&r.status!==207)throw Error('Could not read folder');"
"const x=new DOMParser().parseFromString(await r.text(),'application/xml');const out=[];let self=true;for(const q of x.getElementsByTagNameNS('DAV:','response')){"
"if(self){self=false;continue}"
"const name=q.getElementsByTagNameNS('DAV:','displayname')[0]?.textContent||'Unnamed item';"
"const dir=q.getElementsByTagNameNS('DAV:','collection').length>0;const raw=q.getElementsByTagNameNS('DAV:','getcontentlength')[0]?.textContent;"
"const base=p==='/'?'/':(p.endsWith('/')?p.slice(0,-1):p)+'/';out.push({name,dir,size:dir?null:Number(raw||0),path:base+name})}return out.sort((a,b)=>b.dir-a.dir||a.name.localeCompare(b.name));}"
"async function load(message=''){status.textContent='Loading…';$('location').textContent='Folder: '+path;$('up').disabled=path==='/';try{const list=await entries(path),body=$('items');body.textContent='';"
"if(!list.length)body.innerHTML='<tr><td colspan=2>This folder is empty.</td></tr>';for(const f of list){const tr=document.createElement('tr'),name=document.createElement('td'),b=document.createElement('button');"
"b.type='button';b.textContent=f.name+(f.dir?', folder':'');b.onclick=()=>showItem(f);name.append(b);const size=document.createElement('td');size.textContent=nice(f.size);tr.append(name,size);body.append(tr)}"
"status.textContent=message||list.length+' item'+(list.length===1?'':'s')+'.'}catch(e){status.textContent=e.message}}"
"function showItem(f){selected=f;$('dialogTitle').textContent=f.name;$('itemDetails').textContent=f.dir?'Folder':nice(f.size);$('newName').value=f.name;$('openItem').textContent=f.dir?'Open folder':'Download';$('itemDialog').showModal()}"
"$('openItem').onclick=()=>{const f=selected;if(!f)return;if(f.dir){$('itemDialog').close();path=f.path+'/';load()}else{const a=document.createElement('a');a.href=url(f.path);a.download=f.name;a.click()}};"
"async function remove(f){const warning=f.dir?'Really delete the folder '+f.name+' and everything inside it? This cannot be undone.':'Really delete '+f.name+'? This cannot be undone.';if(!confirm(warning))return;$('itemDialog').close();status.textContent='Deleting '+f.name+'…';try{await removeDirect(f);await load(f.name+' deleted.')}catch(e){status.textContent=e.message}}"
"async function removeDirect(f){if(f.dir){for(const child of await entries(f.path+'/'))await removeDirect(child)}const r=await fetch(url(f.path),{method:'DELETE'});if(!r.ok)throw Error('Could not delete '+f.name)}"
"async function makeFolders(folder){const parts=folder.split('/').filter(Boolean);let made='/';for(const part of parts){made+=part+'/';const exists=await fetch(url(made),{method:'HEAD'});if(exists.ok)continue;const r=await fetch(url(made),{method:'MKCOL'});if(!r.ok)throw Error('Could not create folder '+part)}}"
"$('newFolder').onclick=async()=>{const name=prompt('New folder name');if(!name)return;const clean=name.trim().replace(/^\\/+|\\/+$/g,'');if(!clean||clean.split('/').some(x=>!x||x==='.'||x==='..')){status.textContent='Enter a valid folder name.';return}status.textContent='Creating folder…';try{await makeFolders(path+clean+'/');await load('Folder '+clean+' created.')}catch(e){status.textContent=e.message}};"
"$('deleteItem').onclick=()=>selected&&remove(selected);$('renameItem').onclick=async()=>{const f=selected,requested=$('newName').value.trim().replace(/\\\\/g,'/').replace(/^\\/+|\\/+$/g,'');const parts=requested.split('/');if(!f||!requested||parts.some(x=>!x||x==='.'||x==='..')){status.textContent='Enter a valid name or relative path.';return}"
"const slash=f.path.lastIndexOf('/'),base=f.path.slice(0,slash+1),destination=base+requested,destSlash=destination.lastIndexOf('/');status.textContent='Moving '+f.name+'…';try{if(destSlash>0)await makeFolders(destination.slice(0,destSlash+1));const r=await fetch(url(f.path),{method:'MOVE',headers:{Destination:location.origin+url(destination)}});if(!r.ok)throw Error('Could not rename or move '+f.name);$('itemDialog').close();await load(f.name+' moved to '+requested+'.')}catch(e){status.textContent=e.message}};"
"async function upload(files){files=[...files];if(!files.length)return;$('progressBox').hidden=false;progress.max=files.length;progress.value=0;progress.setAttribute('aria-valuetext','0 of '+files.length+' files');status.textContent='Uploading files…';let done=0;try{for(const f of files){"
"const r=await fetch(url(path+f.name),{method:'PUT',body:f});if(!r.ok)throw Error('Could not upload '+f.name);progress.value=++done;progress.setAttribute('aria-valuetext',done+' of '+files.length+' files')}await load(done+' file'+(done===1?'':'s')+' uploaded.')}catch(e){status.textContent=e.message}finally{$('progressBox').hidden=true;$('files').value=''}}"
"$('files').onchange=e=>upload(e.target.files);$('refresh').onclick=load;$('up').onclick=()=>{const clean=path.endsWith('/')?path.slice(0,-1):path;const a=clean.split('/');a.pop();path=a.join('/')+'/';if(path==='//')path='/';load()};"
"const drop=$('drop');for(const n of ['dragenter','dragover'])drop.addEventListener(n,e=>{e.preventDefault();drop.classList.add('over')});"
"for(const n of ['dragleave','drop'])drop.addEventListener(n,e=>{e.preventDefault();drop.classList.remove('over')});drop.addEventListener('drop',e=>upload(e.dataTransfer.files));"
"document.addEventListener('paste',e=>{if(e.clipboardData?.files?.length){e.preventDefault();upload(e.clipboardData.files)}});load();"
"</script></body></html>";

esp_err_t transfer_web_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html; charset=utf-8");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    return httpd_resp_send(req, page, sizeof(page) - 1);
}

