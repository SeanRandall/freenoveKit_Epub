#include "transfer_web.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "nvs.h"
#include "speech_engine.h"

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
"<a href=/settings>Reader settings</a><a href=/substitutions>Pronunciation substitutions</a>"
"<label>Upload files <input id=files type=file multiple></label></div>"
"<div id=drop class=drop tabindex=0><strong>Drop files here</strong> or paste files copied from Explorer. "
"<span class=muted>Pasting depends on browser support.</span></div>"
"<p id=status role=status aria-live=polite></p><div id=progressBox hidden>"
"<label for=progress>Upload progress</label> <progress id=progress max=100></progress></div>"
"<p id=itemHelp class=muted>Activate a folder to open it. Use the context menu, Shift+F10 or a long press for folder actions.</p>"
"<table aria-describedby=itemHelp><thead><tr><th scope=col>Name</th><th scope=col>Size</th></tr></thead>"
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
"b.type='button';b.textContent=f.name+(f.dir?', folder':'');b.onclick=()=>f.dir?openFolder(f):showItem(f);b.oncontextmenu=e=>{e.preventDefault();showItem(f)};b.onkeydown=e=>{if(e.key==='ContextMenu'||(e.shiftKey&&e.key==='F10')){e.preventDefault();showItem(f)}};name.append(b);const size=document.createElement('td');size.textContent=nice(f.size);tr.append(name,size);body.append(tr)}"
"status.textContent=message||list.length+' item'+(list.length===1?'':'s')+'.'}catch(e){status.textContent=e.message}}"
"function openFolder(f){path=f.path+'/';load()}"
"function showItem(f){selected=f;$('dialogTitle').textContent=f.name;$('itemDetails').textContent=f.dir?'Folder':nice(f.size);$('newName').value=f.name;$('openItem').textContent=f.dir?'Open folder':'Download';$('itemDialog').showModal()}"
"$('openItem').onclick=()=>{const f=selected;if(!f)return;if(f.dir){$('itemDialog').close();openFolder(f)}else{const a=document.createElement('a');a.href=url(f.path);a.download=f.name;a.click()}};"
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

static char active_engine[32] = "engine";
static char active_engine_storage[40] = "engine";
static transfer_web_preview_fn preview_callback;
static void *preview_context;

static const char *canonical_engine(const char *id)
{
    if (!strcmp(id, "oevv")) return "openevv";
    if (!strcmp(id, "dectalk-43-private")) return "dectalk4.3";
    if (!strcmp(id, "dectalk-dtc01")) return "dectalk";
    if (!strncmp(id, "monologue-", 10)) return "monologue";
    if (!strncmp(id, "doubletalk-", 11)) return "doubletalk";
    if (!strncmp(id, "openbst", 7)) return "openbst";
    if (!strncmp(id, "truvoice", 8)) return "truvoice";
    if (!strncmp(id, "apollo2", 7)) return "apollo";
    if (!strncmp(id, "prose2000", 9)) return "prose2000";
    return id;
}

void transfer_web_configure(const char *engine_storage_id,
                            transfer_web_preview_fn preview, void *context)
{
    strlcpy(active_engine_storage, engine_storage_id,
            sizeof(active_engine_storage));
    strlcpy(active_engine, canonical_engine(engine_storage_id),
            sizeof(active_engine));
    preview_callback = preview;
    preview_context = context;
}

static const char substitutions_page[] =
"<!doctype html><html lang=en><head><meta charset=utf-8>"
"<meta name=viewport content='width=device-width,initial-scale=1'>"
"<title>Retroreader substitutions</title><style>"
":root{color-scheme:light dark;font:100% system-ui,sans-serif}body{max-width:72rem;margin:auto;padding:1rem;line-height:1.5}"
"table{width:100%;border-collapse:collapse;margin:.5rem 0 1rem}th,td{padding:.4rem;text-align:left;border-bottom:1px solid #888}"
"input[type=text]{box-sizing:border-box;width:100%;font:inherit;padding:.4rem}button,a{font:inherit;padding:.45rem;margin:.15rem}.actions{display:flex;gap:.4rem;flex-wrap:wrap}"
"</style></head><body><main><h1>Pronunciation substitutions</h1>"
"<p><a href=/web>Back to SD card files</a></p><p>Changes take effect after restarting Retroreader. Commands in the engine table are sent only to <strong id=engineName></strong>.</p>"
"<p id=status role=status aria-live=polite></p><div id=tables></div>"
"</main><script>const $=x=>document.getElementById(x),status=$('status');let sets={};"
"function section(scope,title){const s=document.createElement('section'),h=document.createElement('h2');h.textContent=title;s.append(h);const t=document.createElement('table');t.innerHTML='<thead><tr><th scope=col>Mark</th><th scope=col>Text to find</th><th scope=col>Replacement</th><th scope=col>Preview</th></tr></thead><tbody></tbody>';s.append(t);const a=document.createElement('div');a.className='actions';for(const [label,fn] of [['Add entry',()=>add(scope)],['Delete marked',()=>removeMarked(scope)],['Save changes',()=>save(scope)]]){const b=document.createElement('button');b.type='button';b.textContent=label;b.onclick=fn;a.append(b)}s.append(a);$('tables').append(s);sets[scope]={body:t.tBodies[0],comments:[]};}"
"function row(scope,from='',to=''){const tr=document.createElement('tr'),mark=document.createElement('input'),a=document.createElement('input'),b=document.createElement('input'),play=document.createElement('button');mark.type='checkbox';mark.setAttribute('aria-label','Mark '+(from||'new entry'));a.type=b.type='text';a.value=from;b.value=to;a.setAttribute('aria-label','Text to find');b.setAttribute('aria-label','Replacement');play.type='button';play.textContent='Play replacement';play.onclick=()=>preview(b.value);for(const x of [mark,a,b,play]){const td=document.createElement('td');td.append(x);tr.append(td)}sets[scope].body.append(tr);a.focus()}"
"function add(scope){row(scope)}function removeMarked(scope){for(const tr of [...sets[scope].body.rows])if(tr.cells[0].firstChild.checked)tr.remove()}"
"function parse(scope,text){const s=sets[scope];s.body.textContent='';s.comments=[];for(const line of text.split(/\\r?\\n/)){if(!line)continue;if(line.startsWith('#')){s.comments.push(line);continue}const p=line.indexOf('\\t');if(p>0)row(scope,line.slice(0,p),line.slice(p+1))}}"
"function content(scope){const lines=[...sets[scope].comments];for(const tr of sets[scope].body.rows){const a=tr.cells[1].firstChild.value,b=tr.cells[2].firstChild.value;if(a)lines.push(a+'\\t'+b)}return lines.join('\\n')+(lines.length?'\\n':'')}"
"async function load(){status.textContent='Loading…';try{const info=await (await fetch('/substitutions?action=info')).json();$('engineName').textContent=info.engine;section('global','Global substitutions');section('engine','Substitutions for '+info.engine);for(const scope of ['global','engine']){const r=await fetch('/substitutions?action=load&scope='+scope);if(!r.ok)throw Error('Could not load '+scope+' substitutions');parse(scope,await r.text())}status.textContent='Substitutions loaded.'}catch(e){status.textContent=e.message}}"
"async function save(scope){status.textContent='Saving…';const body=new URLSearchParams({scope,content:content(scope)});try{const r=await fetch('/substitutions?action=save',{method:'POST',body});if(!r.ok)throw Error(await r.text()||'Could not save');status.textContent=(scope==='global'?'Global':'Engine')+' substitutions saved.'}catch(e){status.textContent=e.message}}"
"async function preview(text){if(!text)return;status.textContent='Playing replacement…';try{const r=await fetch('/substitutions?action=preview',{method:'POST',body:new URLSearchParams({replacement:text})});if(!r.ok)throw Error(await r.text()||'Could not play replacement');status.textContent='Replacement queued on Retroreader.'}catch(e){status.textContent=e.message}}load();"
"</script></body></html>";

static void substitution_path(bool engine, char path[96])
{
    if (engine)
        snprintf(path, 96, "/sdcard/.rr/substitutions.%s.tsv", active_engine);
    else
        strlcpy(path, "/sdcard/.rr/substitutions.tsv", 96);
}

static void url_decode(char *text)
{
    char *source = text, *destination = text;
    while (*source) {
        if (*source == '+') { *destination++ = ' '; ++source; continue; }
        if (*source == '%' && isxdigit((unsigned char)source[1])
            && isxdigit((unsigned char)source[2])) {
            char hex[3] = { source[1], source[2], 0 };
            *destination++ = (char)strtoul(hex, NULL, 16);
            source += 3;
            continue;
        }
        *destination++ = *source++;
    }
    *destination = 0;
}

static char *form_value(char *body, const char *name)
{
    size_t length = strlen(name);
    for (char *field = body; field && *field;) {
        char *next = strchr(field, '&');
        if (next) *next++ = 0;
        if (!strncmp(field, name, length) && field[length] == '=') {
            char *value = field + length + 1;
            url_decode(value);
            return value;
        }
        field = next;
    }
    return NULL;
}

static void decode_replacement_escapes(char *text)
{
    unsigned char *source = (unsigned char *)text;
    unsigned char *destination = (unsigned char *)text;
    while (*source) {
        if (*source++ != '\\') { *destination++ = source[-1]; continue; }
        if (*source == '\\') { *destination++ = *source++; continue; }
        if (*source == 'e') { *destination++ = 0x1b; ++source; continue; }
        if (*source == 'r') { *destination++ = '\r'; ++source; continue; }
        if (*source == 'n') { *destination++ = '\n'; ++source; continue; }
        if (*source == 't') { *destination++ = '\t'; ++source; continue; }
        if (*source == 'x' && isxdigit(source[1]) && isxdigit(source[2])) {
            char hex[3] = { (char)source[1], (char)source[2], 0 };
            unsigned value = (unsigned)strtoul(hex, NULL, 16);
            if (value) *destination++ = (unsigned char)value;
            source += 3;
            continue;
        }
        *destination++ = '\\';
    }
    *destination = 0;
}

static esp_err_t receive_form(httpd_req_t *req, char **body)
{
    if (req->content_len <= 0 || req->content_len > 98304)
        return ESP_ERR_INVALID_SIZE;
    *body = malloc((size_t)req->content_len + 1);
    if (!*body) return ESP_ERR_NO_MEM;
    size_t received = 0;
    while (received < (size_t)req->content_len) {
        int count = httpd_req_recv(req, *body + received,
                                   req->content_len - received);
        if (count <= 0) { free(*body); *body = NULL; return ESP_FAIL; }
        received += (size_t)count;
    }
    (*body)[received] = 0;
    return ESP_OK;
}

static const char settings_page[] =
"<!doctype html><html lang=en><head><meta charset=utf-8>"
"<meta name=viewport content='width=device-width,initial-scale=1'>"
"<title>Retroreader settings</title><style>"
":root{color-scheme:light dark;font:100% system-ui,sans-serif}body{max-width:48rem;margin:auto;padding:1rem;line-height:1.5}"
"label{display:block;margin:.7rem 0}select,input,button{font:inherit;padding:.4rem}fieldset{margin:1rem 0;padding:1rem}"
"</style></head><body><main><h1>Reader settings</h1>"
"<p><a href=/web>Back to SD card files</a> · <a href=/substitutions>Pronunciation substitutions</a></p>"
"<p>Settings take effect when you leave file transfer and return to the reader.</p>"
"<form id=settings><div id=fields></div><button type=submit>Save settings</button></form>"
"<p id=status role=status aria-live=polite></p></main><script>"
"const $=x=>document.getElementById(x),form=$('settings'),fields=$('fields'),status=$('status');"
"function select(name,label,options,value){const l=document.createElement('label');l.textContent=label+' ';const s=document.createElement('select');s.name=name;for(const [v,t] of options){const o=document.createElement('option');o.value=v;o.textContent=t;o.selected=String(v)===String(value);s.append(o)}l.append(s);return l}"
"function number(name,label,min,max,step,value){const l=document.createElement('label');l.textContent=label+' ';const i=document.createElement('input');Object.assign(i,{type:'number',name,min,max,step,value});l.append(i);return l}"
"function group(title){const f=document.createElement('fieldset'),l=document.createElement('legend');l.textContent=title;f.append(l);fields.append(f);return f}"
"async function load(){status.textContent='Loading settings…';try{const r=await fetch('/settings?action=load');if(!r.ok)throw Error('Could not load settings');const x=await r.json();fields.textContent='';let g=group('Reading');g.append(select('sleeptime','Sleep timer',x.sleepOptions,x.sleeptime),select('libsort','Sort files by',[[0,'Alphabetical'],[1,'Date'],[2,'Size']],x.libsort),select('sectpause','Pause between sections',[[0,'Off'],[1,'On']],x.sectpause),select('startup','On startup',[[0,'Library'],[1,'Resume reading']],x.startup),select('unlockact','Unlock action',[[0,'Do nothing'],[1,'Speak time'],[2,'Speak battery'],[3,'Speak both'],[4,'Resume playback']],x.unlockact),select('recents','Recent files',[[0,'Off'],[1,'On']],x.recents),select('instant','Book opening',[[0,'Indexed'],[1,'Instant']],x.instant),select('audiofold','Audio folders as playlists',[[0,'Off'],[1,'On']],x.audiofold),select('uisound','Interface sounds',[[0,'Off'],[1,'On']],x.uisound),select('sleepkey','Keypress resets sleep timer',[[0,'Off'],[1,'On']],x.sleepkey),select('sleepboot','Start sleep timer on boot',[[0,'Off'],[1,'On']],x.sleepboot));"
"g=group('Speech — '+x.engine);g.append(number('volume','Playback volume',2,100,2,x.volume));for(const p of x.speech){let c;if(p.labels)c=select('speech'+p.id,p.name,p.labels.map((t,i)=>[p.min+i,t]),p.value);else if(p.boolean)c=select('speech'+p.id,p.name,[[0,p.off||'Off'],[1,p.on||'On']],p.value);else c=number('speech'+p.id,p.name,p.min,p.max,p.step,p.value);g.append(c)}"
"g=group('Playing screen status items');for(const [i,n] of x.statusNames.entries()){const l=document.createElement('label'),c=document.createElement('input');c.type='checkbox';c.name='status'+i;c.checked=!!(x.statusmask&(1<<i));l.append(c,' '+n);g.append(l)}status.textContent='Settings loaded.'}catch(e){status.textContent=e.message}}"
"form.onsubmit=async e=>{e.preventDefault();status.textContent='Saving settings…';const data=new URLSearchParams(new FormData(form));for(let i=0;i<9;i++)data.set('status'+i,form.elements['status'+i].checked?'1':'0');try{const r=await fetch('/settings?action=save',{method:'POST',body:data});if(!r.ok)throw Error(await r.text()||'Could not save settings');status.textContent='Settings saved.'}catch(e){status.textContent=e.message}};load();"
"</script></body></html>";

static void settings_speech_key(char key[16], const char *suffix)
{
    size_t id_length = strlen(active_engine_storage);
    size_t suffix_length = strlen(suffix);
    if (id_length + suffix_length + 1 <= 15) {
        memcpy(key, active_engine_storage, id_length);
        key[id_length] = '_';
        memcpy(key + id_length + 1, suffix, suffix_length + 1);
        return;
    }
    uint32_t hash = UINT32_C(2166136261);
    for (const unsigned char *p =
             (const unsigned char *)active_engine_storage; *p; ++p) {
        hash ^= *p;
        hash *= UINT32_C(16777619);
    }
    int prefix = snprintf(key, 16, "e%08lx_", (unsigned long)hash);
    if (prefix > 0 && prefix < 16)
        strlcpy(key + prefix, suffix, 16 - (size_t)prefix);
}

static int32_t settings_get_i32(nvs_handle_t prefs, const char *key,
                                int32_t fallback)
{
    int32_t value;
    return nvs_get_i32(prefs, key, &value) == ESP_OK ? value : fallback;
}

static bool settings_append(char **at, size_t *left, const char *format, ...)
{
    va_list arguments;
    va_start(arguments, format);
    int written = vsnprintf(*at, *left, format, arguments);
    va_end(arguments);
    if (written < 0 || (size_t)written >= *left) return false;
    *at += written;
    *left -= (size_t)written;
    return true;
}

static esp_err_t settings_send_json(httpd_req_t *req)
{
    nvs_handle_t prefs;
    if (nvs_open("reader", NVS_READONLY, &prefs) != ESP_OK)
        return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
                                   "Could not open reader settings");
    char *json = malloc(12288);
    if (!json) { nvs_close(prefs); return ESP_ERR_NO_MEM; }
    char *at = json;
    size_t left = 12288;
    uint32_t status_mask = (1U << 0) | (1U << 3) | (1U << 4);
    (void)nvs_get_u32(prefs, "statusmask", &status_mask);
    static const unsigned sleep_minutes[] = {0, 5, 10, 15, 30, 45, 60, 90};
    settings_append(&at, &left,
        "{\"engine\":\"%s\",\"volume\":%ld,\"sleeptime\":%ld,"
        "\"libsort\":%ld,\"sectpause\":%ld,\"startup\":%ld,"
        "\"unlockact\":%ld,\"recents\":%ld,\"instant\":%ld,\"audiofold\":%ld,"
        "\"uisound\":%ld,\"sleepkey\":%ld,\"sleepboot\":%ld,"
        "\"statusmask\":%lu,\"sleepOptions\":[",
        active_engine,
        (long)settings_get_i32(prefs, "volume", 50),
        (long)settings_get_i32(prefs, "sleeptime", 0),
        (long)settings_get_i32(prefs, "libsort", 0),
        (long)settings_get_i32(prefs, "sectpause", 0),
        (long)settings_get_i32(prefs, "startup", 0),
        (long)settings_get_i32(prefs, "unlockact", 0),
        (long)settings_get_i32(prefs, "recents", 1),
        (long)settings_get_i32(prefs, "instant", 0),
        (long)settings_get_i32(prefs, "audiofold", 0),
        (long)settings_get_i32(prefs, "uisound", 1),
        (long)settings_get_i32(prefs, "sleepkey", 0),
        (long)settings_get_i32(prefs, "sleepboot", 0),
        (unsigned long)status_mask);
    for (size_t i = 0; i < sizeof(sleep_minutes) / sizeof(sleep_minutes[0]); ++i) {
        if (!i) settings_append(&at, &left, "[0,\"Off\"]");
        else settings_append(&at, &left, ",[%u,\"%u minutes\"]",
                             (unsigned)i, sleep_minutes[i]);
    }
    settings_append(&at, &left,
        "],\"statusNames\":[\"Time\",\"Date\",\"Remaining section duration\","
        "\"Remaining file duration\",\"Battery level\",\"Battery time estimate\","
        "\"Sleep time remaining\",\"Remaining SD card space\",\"Device temperature\"],"
        "\"speech\":[");
    size_t setting_count = speech_engine_setting_count(NULL);
    bool first = true;
    for (size_t i = 0; i < setting_count; ++i) {
        speech_setting_info_t info;
        if (!speech_engine_setting_info(NULL, i, &info)) continue;
        char suffix[5], key[16];
        snprintf(suffix, sizeof(suffix), "s%u", (unsigned)info.id);
        settings_speech_key(key, suffix);
        int32_t value = settings_get_i32(prefs, key, info.default_value);
        if (info.id == SPEECH_SETTING_RATE) {
            settings_speech_key(key, "rate");
            value = settings_get_i32(prefs, key, value);
        }
        settings_append(&at, &left,
            "%s{\"id\":%u,\"name\":\"%s\",\"min\":%d,\"max\":%d,"
            "\"step\":%d,\"value\":%ld,\"boolean\":%s,"
            "\"off\":\"%s\",\"on\":\"%s\"",
            first ? "" : ",", (unsigned)info.id, info.name,
            info.minimum, info.maximum, info.step, (long)value,
            info.boolean_value ? "true" : "false",
            info.false_label ? info.false_label : "Off",
            info.true_label ? info.true_label : "On");
        first = false;
        if (info.value_labels) {
            settings_append(&at, &left, ",\"labels\":[");
            for (int value_index = info.minimum; value_index <= info.maximum;
                 ++value_index)
                settings_append(&at, &left, "%s\"%s\"",
                    value_index == info.minimum ? "" : ",",
                    info.value_labels[value_index - info.minimum]);
            settings_append(&at, &left, "]");
        }
        settings_append(&at, &left, "}");
    }
    settings_append(&at, &left, "]}");
    nvs_close(prefs);
    httpd_resp_set_type(req, "application/json");
    esp_err_t result = httpd_resp_send(req, json, HTTPD_RESP_USE_STRLEN);
    free(json);
    return result;
}

static bool settings_valid_speech(unsigned id, int value,
                                  speech_setting_info_t *found)
{
    size_t count = speech_engine_setting_count(NULL);
    for (size_t i = 0; i < count; ++i) {
        speech_setting_info_t info;
        if (speech_engine_setting_info(NULL, i, &info)
            && (unsigned)info.id == id && value >= info.minimum
            && value <= info.maximum
            && (!info.step || (value - info.minimum) % info.step == 0)) {
            if (found) *found = info;
            return true;
        }
    }
    return false;
}

static esp_err_t settings_save_form(httpd_req_t *req)
{
    char *body = NULL;
    esp_err_t result = receive_form(req, &body);
    if (result != ESP_OK) return result;
    nvs_handle_t prefs;
    if (nvs_open("reader", NVS_READWRITE, &prefs) != ESP_OK) {
        free(body);
        return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
                                   "Could not open reader settings");
    }
    uint32_t status_mask = 0;
    char *save = NULL;
    for (char *field = strtok_r(body, "&", &save); field;
         field = strtok_r(NULL, "&", &save)) {
        char *equals = strchr(field, '=');
        if (!equals) continue;
        *equals++ = 0;
        url_decode(field);
        url_decode(equals);
        char *end = NULL;
        long value = strtol(equals, &end, 10);
        if (!end || *end) continue;
        if (!strncmp(field, "status", 6)) {
            unsigned bit = (unsigned)strtoul(field + 6, &end, 10);
            if (end && !*end && bit < 9 && value) status_mask |= 1U << bit;
            continue;
        }
        if (!strncmp(field, "speech", 6)) {
            unsigned id = (unsigned)strtoul(field + 6, &end, 10);
            speech_setting_info_t info;
            if (!end || *end || !settings_valid_speech(id, (int)value, &info))
                continue;
            char suffix[5], key[16];
            snprintf(suffix, sizeof(suffix), "s%u", id);
            settings_speech_key(key, suffix);
            (void)nvs_set_i32(prefs, key, (int32_t)value);
            if (id == SPEECH_SETTING_RATE) {
                settings_speech_key(key, "rate");
                (void)nvs_set_i32(prefs, key, (int32_t)value);
            }
            continue;
        }
        struct { const char *name; int minimum, maximum; } const simple[] = {
            {"volume", 2, 100}, {"sleeptime", 0, 7}, {"libsort", 0, 2},
            {"sectpause", 0, 1}, {"startup", 0, 1}, {"unlockact", 0, 4},
            {"recents", 0, 1}, {"instant", 0, 1}, {"audiofold", 0, 1},
            {"uisound", 0, 1},
            {"sleepkey", 0, 1}, {"sleepboot", 0, 1},
        };
        for (size_t i = 0; i < sizeof(simple) / sizeof(simple[0]); ++i)
            if (!strcmp(field, simple[i].name) && value >= simple[i].minimum
                && value <= simple[i].maximum) {
                (void)nvs_set_i32(prefs, field, (int32_t)value);
                break;
            }
    }
    (void)nvs_set_u32(prefs, "statusmask", status_mask);
    result = nvs_commit(prefs);
    nvs_close(prefs);
    free(body);
    if (result != ESP_OK)
        return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
                                   "Could not save settings");
    return httpd_resp_sendstr(req, "Settings saved");
}

esp_err_t transfer_settings_handler(httpd_req_t *req)
{
    char query[48] = {0}, action[12] = {0};
    if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK)
        (void)httpd_query_key_value(query, "action", action, sizeof(action));
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    if (!action[0]) {
        httpd_resp_set_type(req, "text/html; charset=utf-8");
        return httpd_resp_send(req, settings_page, sizeof(settings_page) - 1);
    }
    if (!strcmp(action, "load") && req->method == HTTP_GET)
        return settings_send_json(req);
    if (!strcmp(action, "save") && req->method == HTTP_POST)
        return settings_save_form(req);
    return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST,
                               "Unknown settings action");
}

esp_err_t transfer_substitutions_handler(httpd_req_t *req)
{
    char query[96] = { 0 }, action[16] = { 0 }, scope[16] = { 0 };
    if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK) {
        (void)httpd_query_key_value(query, "action", action, sizeof(action));
        (void)httpd_query_key_value(query, "scope", scope, sizeof(scope));
    }
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    if (!action[0]) {
        httpd_resp_set_type(req, "text/html; charset=utf-8");
        return httpd_resp_send(req, substitutions_page,
                               sizeof(substitutions_page) - 1);
    }
    if (!strcmp(action, "info")) {
        char json[80];
        snprintf(json, sizeof(json), "{\"engine\":\"%s\"}", active_engine);
        httpd_resp_set_type(req, "application/json");
        return httpd_resp_sendstr(req, json);
    }
    if (!strcmp(action, "load")) {
        char path[96]; substitution_path(!strcmp(scope, "engine"), path);
        FILE *file = fopen(path, "rb");
        httpd_resp_set_type(req, "text/tab-separated-values; charset=utf-8");
        if (!file) return httpd_resp_send(req, "", 0);
        char buffer[1024]; size_t count;
        while ((count = fread(buffer, 1, sizeof(buffer), file)) != 0)
            if (httpd_resp_send_chunk(req, buffer, count) != ESP_OK) break;
        fclose(file);
        return httpd_resp_send_chunk(req, NULL, 0);
    }
    if (req->method != HTTP_POST) return httpd_resp_send_err(
        req, HTTPD_405_METHOD_NOT_ALLOWED, "POST required");
    char *body = NULL;
    if (receive_form(req, &body) != ESP_OK)
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid request");
    esp_err_t result = ESP_OK;
    if (!strcmp(action, "preview")) {
        char *replacement = form_value(body, "replacement");
        if (!replacement || !preview_callback) result = ESP_ERR_INVALID_STATE;
        else { decode_replacement_escapes(replacement); if (!preview_callback(
                   replacement, preview_context)) result = ESP_FAIL; }
    } else if (!strcmp(action, "save")) {
        char *content = form_value(body, "content");
        /* content is second in URLSearchParams. Find it first because the
           compact form parser terminates preceding fields in place. */
        char *selected_scope = form_value(body, "scope");
        if (!selected_scope || !content) result = ESP_ERR_INVALID_ARG;
        else {
            char path[96], temporary[104];
            substitution_path(!strcmp(selected_scope, "engine"), path);
            snprintf(temporary, sizeof(temporary), "%s.upload", path);
            FILE *file = fopen(temporary, "wb");
            if (!file) result = ESP_FAIL;
            else {
                size_t bytes = strlen(content);
                bool written = fwrite(content, 1, bytes, file) == bytes;
                if (fclose(file) != 0) written = false;
                if (written) (void)remove(path);
                if (!written || rename(temporary, path) != 0)
                    result = ESP_FAIL;
                if (result != ESP_OK) (void)remove(temporary);
            }
        }
    } else result = ESP_ERR_NOT_SUPPORTED;
    free(body);
    if (result != ESP_OK)
        return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
                                   "Substitution operation failed");
    return httpd_resp_sendstr(req, "OK");
}

