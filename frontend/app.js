/* ChronoTrace interface. All data comes from the /api endpoints; nothing is preloaded.
   Response shapes expected:
   GET /events -> {events:[{event_id,timestamp,entity_id,event_type,severity}]}
   POST /import -> {counts:{events,entities,relationships},rejected:[{file,line,reason}]}
   GET /graph, /entities/:id -> {nodes:[{id,type,name}],edges:[{record_id,source,target,relationship}]}
   GET /path -> {found,nodes:[{id,name}],edges:[{record_id,source,target,relationship}],cost}
   GET /groups -> {groups:[{id,size,members:[]}],linking:[{id,name,reason}]}
   GET /bursts -> {bursts:[{start,end,count,max_severity,event_ids:[]}]}
   POST /integrity/verify -> {verified_at,manifest_match,results:[{file,baseline,current,match}]}
   POST /reports -> {id};  GET /reports/:id -> {title,generated_at,sections:[{heading,items:[{text,record_ids:[]}]}]}
   GET /search?prefix= -> {results:[{id,label}]} */
const S={loaded:false,events:[],sel:new Set(),tab:'overview',focus:'',groups:[]};
const $=(s,e=document)=>e.querySelector(s);
const esc=s=>String(s??'').replace(/[&<>"']/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));
let tt;const toast=m=>{const t=$('#toast');if(!t)return;t.textContent=m;t.classList.add('on');clearTimeout(tt);tt=setTimeout(()=>t.classList.remove('on'),4000)};
async function api(p,o){
  const r=await fetch('/api'+p,o).catch(()=>{throw new Error('Cannot reach the server. Check that it is running.')});
  const j=await r.json().catch(()=>({}));
  if(!r.ok)throw new Error(j.error?.message||j.error||j.message||'Request failed ('+r.status+')');return j}
const post=(p,b)=>api(p,{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(b||{})});
const guard=fn=>async(...a)=>{try{await fn(...a)}catch(e){toast(e.message)}};
const sev=n=>`<span class="badge s${esc(n)}">Level ${esc(n)}</span>`;
const ART='<svg viewBox="0 0 380 260" aria-hidden="true"><g stroke="#b6f76b" stroke-width="3" stroke-linecap="round"><path d="M70 60L190 130M190 130L310 70M190 130L120 205M190 130L300 200M310 70L300 200" fill="none"/></g><g stroke="#fff" stroke-width="3"><circle cx="70" cy="60" r="20" fill="#ffb020"/><circle cx="190" cy="130" r="26" fill="#8b74ff"/><circle cx="310" cy="70" r="20" fill="#4f8cff"/><circle cx="120" cy="205" r="18" fill="#ff6b7a"/><circle cx="300" cy="200" r="22" fill="#b6f76b"/></g></svg>';
const empty=()=>`<div class="empty"><div><h2>No evidence imported yet</h2><p>Upload events.csv, entities.csv and relationships.csv to begin.</p><button class="btn pri" data-imp>Upload evidence files</button></div>${ART}</div>`;
const none=t=>`<p class="muted">${t}</p>`;
const link=(id,go)=>`<button class="link" data-go="${esc(go)}" data-id="${esc(id)}">${esc(id)}</button>`;
const ids=a=>(a||[]).map(i=>`<span class="chip">${esc(i)}</span>`).join('');

const TABS=[['overview','Overview'],['timeline','Chronological timeline'],['graph','Evidence graph explorer'],['path','Relationship path tracer'],['groups','Clusters & critical connectors'],['bursts','Activity burst detection'],['integrity','Evidence integrity verification'],['report','Investigation report']];
const VIEWS={overview:vOverview,timeline:vTimeline,graph:vGraph,path:vPath,groups:vGroups,bursts:vBursts,integrity:vIntegrity,report:vReport};
function tabs(){$('#tabs').innerHTML=TABS.map(([k,l])=>`<button role="tab" data-tab="${k}" aria-selected="${k===S.tab}">${l}</button>`).join('')}
function show(tab,focus){if(tab)S.tab=tab;if(focus!==undefined)S.focus=focus;tabs();
  if(!S.loaded)$('#view').innerHTML=empty();else guard(VIEWS[S.tab])()}
async function refresh(){
  try{S.events=(await api('/events')).events||[];S.loaded=S.events.length>0;
    $('#cEv').textContent=S.events.length;
    S.groups=S.loaded?(await api('/groups')).groups||[]:[];$('#cCl').textContent=S.groups.length}
  catch(e){S.loaded=false;toast(e.message)}
  show()}

/* Overview */
function vOverview(){
  const ev=S.events,hi=ev.filter(e=>+e.severity===3),ts=ev.map(e=>e.timestamp).sort();
  const K=[['Events',ev.length],['Entities in events',new Set(ev.map(e=>e.entity_id)).size],['Clusters',S.groups.length],['Level 3 events',hi.length]];
  $('#view').innerHTML=`<div class="kpi">${K.map(([l,v])=>`<div class="card"><div class="mute">${l}</div><div class="big">${v}</div></div>`).join('')}</div>
  <div class="hero"><div><h2>Where to look first</h2><p>Recorded activity runs from ${esc(ts[0])} to ${esc(ts[ts.length-1])} (UTC). Start with the highest severity events, then follow their connections.</p>
  <button class="btn pri" data-tab="timeline">Open timeline</button><button class="btn ghost" data-tab="integrity">Check integrity</button></div>${ART}</div>
  <div class="card"><div class="ph"><h2>Highest severity events</h2><span class="muted">Level 3 only</span></div>${hi.length?`<table><thead><tr><th>Event ID</th><th>Timestamp (UTC)</th><th>Entity</th><th>Event type</th><th>Severity</th></tr></thead><tbody>${hi.map(e=>`<tr><td><b>${esc(e.event_id)}</b></td><td>${esc(e.timestamp)}</td><td>${link(e.entity_id,'graph')}</td><td>${esc(e.event_type)}</td><td>${sev(e.severity)}</td></tr>`).join('')}</tbody></table>`:none('No level 3 events in this dataset.')}</div>`}

/* Timeline */
async function vTimeline(){
  const F=[['type','Event type','e.g. login'],['entity','Associated entity','e.g. U01'],['start','Start (UTC)','YYYY-MM-DD HH:MM:SS'],['end','End (UTC)','YYYY-MM-DD HH:MM:SS']];
  $('#view').innerHTML=`<form class="panel filters" id="f">${F.map(([n,l,p])=>`<label>${l}<input name="${n}" placeholder="${p}"></label>`).join('')}
  <label>Minimum severity<select name="min_severity"><option value="">All</option><option>1</option><option>2</option><option>3</option></select></label>
  <button class="btn pri">Filter timeline</button><button type="reset" class="btn">Reset</button></form>
  <div class="panel"><div class="ph"><h3>Ordered event sequence</h3><span class="muted">Sorted by timestamp, ties broken by event ID</span></div><div id="tl"></div></div>`;
  const run=guard(async()=>{const q=new URLSearchParams(new FormData($('#f')));
    for(const [k,v] of [...q])if(!v)q.delete(k);
    S.events=(await api('/events?'+q)).events||[];draw()});
  $('#f').onsubmit=e=>{e.preventDefault();run()};
  $('#f').onreset=()=>setTimeout(run);
  draw()}
function draw(){const el=$('#tl');if(!el)return;
  el.innerHTML=S.events.length?`<table><thead><tr><th></th><th>Event ID</th><th>Timestamp (UTC)</th><th>Entity</th><th>Event type</th><th>Severity</th><th></th></tr></thead><tbody>${S.events.map(e=>`<tr>
  <td><input type="checkbox" aria-label="Select ${esc(e.event_id)} for report" data-sel="${esc(e.event_id)}" ${S.sel.has(e.event_id)?'checked':''} style="width:auto"></td>
  <td><b>${esc(e.event_id)}</b></td><td>${esc(e.timestamp)}</td><td>${link(e.entity_id,'graph')}</td><td>${esc(e.event_type)}</td><td>${sev(e.severity)}</td>
  <td><button class="btn sm" data-go="graph" data-id="${esc(e.entity_id)}">Trace in graph</button></td></tr>`).join('')}</tbody></table>`:none('No events match these filters.')}

/* Graph */
async function vGraph(){
  $('#view').innerHTML=`<form class="panel filters" id="f"><label>Start entity<input name="entity_id" value="${esc(S.focus)}" placeholder="e.g. U01" required></label>
  <label>Depth<select name="depth"><option>1</option><option>2</option><option selected>3</option></select></label>
  <label>Traversal<select name="mode"><option value="bfs">Level by level</option><option value="dfs">Follow chains</option></select></label>
  <button class="btn pri">Explore</button></form>
  <div style="display:flex;gap:8px;margin:8px 0">
    <button class="btn" type="button" onclick="window.fitGraph&&window.fitGraph()">Fit</button>
    <button class="btn" type="button" onclick="window.relayoutGraph&&window.relayoutGraph()">Re-layout</button>
  </div>
  <div class="split"><div id="cy" style="min-height:480px"></div><div class="panel" id="side"><h3>Details</h3>${none('Select a node or a relationship to see its supporting records.')}</div></div>`;
  const run=guard(async()=>{const q=new URLSearchParams(new FormData($('#f')));
    q.set('include_isolated', '1');
    const g=await api('/graph?'+q);
    if(!(g.nodes||[]).length)return toast('No connections found for that entity.');
    S.focus=$('#f').entity_id.value;
    drawGraph($('#cy'),g,{
      startEntity: S.focus,
      onNodeTap: n=>$('#side').innerHTML=`<h3>${esc(n.name||n.label||n.id)}</h3><p class="muted">${esc(n.type)} &middot; ${esc(n.id)}</p><button class="btn sm" data-go="path" data-id="${esc(n.id)}">Trace a path from here</button>`,
      onEdgeTap: e=>$('#side').innerHTML=`<h3>${esc(e.label||e.relationship)}</h3><p class="muted">Supporting record</p><div class="chips">${ids([e.rec||e.id])}</div>`,
      onBackgroundTap: ()=>$('#side').innerHTML=`<h3>Details</h3><p class="muted">Select a node or a relationship to see its supporting records.</p>`
    })});
  $('#f').onsubmit=e=>{e.preventDefault();run()};
  if(S.focus)run()}

/* Path */
async function vPath(){
  $('#view').innerHTML=`<form class="panel filters" id="f"><label>From<input name="from" value="${esc(S.focus)}" required></label><label>To<input name="to" required></label>
  <label>Shortest by<select name="mode"><option value="hops">Fewest steps</option><option value="time">Smallest time gap</option></select></label><button class="btn pri">Find path</button></form><div id="out"></div>`;
  $('#f').onsubmit=guard(async e=>{e.preventDefault();const r=await api('/path?'+new URLSearchParams(new FormData($('#f'))));
    $('#out').innerHTML=r.found?`<div class="panel"><div class="ph"><h3>Path found</h3><span class="muted">${r.edges.length} step(s)${r.cost!=null?` &middot; cost ${esc(r.cost)}`:''}</span></div><ol class="steps">${r.edges.map(e=>`<li>${esc(e.source)} <b>${esc(e.relationship)}</b> ${esc(e.target)} <div class="chips">Record ${ids([e.record_id])}</div></li>`).join('')}</ol>
    <p class="muted">Each step is an imported relationship record. A path shows a connection in the data, not that one event caused another.</p></div>`:`<div class="panel">${none('No connection exists between these two entities.')}</div>`})}

/* Groups */
async function vGroups(){
  const r=await api('/groups');
  $('#view').innerHTML=`<div class="panel"><div class="ph"><h3>Connected groups</h3><span class="muted">${(r.groups||[]).length} found</span></div><div class="grid">${(r.groups||[]).map(g=>`<div class="panel" style="margin:0"><b>Group ${esc(g.id)}</b> <span class="muted">${esc(g.size)} entities</span><div class="chips" style="margin-top:8px">${(g.members||[]).map(m=>link(m,'graph')).join('')}</div></div>`).join('')}</div></div>
  <div class="panel"><div class="ph"><h3>Critical connectors</h3><span class="muted">Removing one of these would split a group</span></div>${(r.linking||[]).length?`<table><tbody>${r.linking.map(l=>`<tr><td>${link(l.id,'graph')}</td><td>${esc(l.name)}</td><td class="muted">${esc(l.reason||'')}</td></tr>`).join('')}</tbody></table>`:none('No critical connectors found.')}</div>`}

/* Bursts */
async function vBursts(){
  $('#view').innerHTML=`<form class="panel filters" id="f"><label>Window (minutes)<input name="window" type="number" min="1" value="5"></label><label>Minimum events in window<input name="threshold" type="number" min="1" value="3"></label><button class="btn pri">Detect bursts</button></form><div id="out"></div>`;
  const run=guard(async()=>{const r=await api('/bursts?'+new URLSearchParams(new FormData($('#f'))));
    $('#out').innerHTML=`<div class="panel">${(r.bursts||[]).length?`<table><thead><tr><th>Window start</th><th>Window end</th><th>Events</th><th>Peak severity</th><th>Records</th></tr></thead><tbody>${r.bursts.map(b=>`<tr><td>${esc(b.start)}</td><td>${esc(b.end)}</td><td>${esc(b.count)}</td><td>${sev(b.max_severity)}</td><td><div class="chips">${ids(b.event_ids)}</div></td></tr>`).join('')}</tbody></table><p class="muted">Bursts are observations about timing only.</p>`:none('No bursts match these settings.')}</div>`});
  $('#f').onsubmit=e=>{e.preventDefault();run()};run()}

/* Integrity */
async function vIntegrity(){
  $('#view').innerHTML=`<div class="panel"><div class="ph"><h3>Source file integrity</h3><button class="btn pri" id="ver">Verify now</button></div><div id="out">${none('Compare each imported file with the hash stored when it was imported.')}</div></div>`;
  $('#ver').onclick=guard(async()=>{const r=await post('/integrity/verify');
    $('#out').innerHTML=`<table><thead><tr><th>File</th><th>Stored hash</th><th>Current hash</th><th>Result</th></tr></thead><tbody>${(r.results||[]).map(x=>`<tr><td>${esc(x.file)}</td><td><code>${esc(x.baseline)}</code></td><td><code>${esc(x.current)}</code></td><td><span class="badge ${x.match?'ok':'bad'}">${x.match?'Match':'Changed'}</span></td></tr>`).join('')}</tbody></table>
    <p class="muted">Checked ${esc(r.verified_at)}. A match shows the file is unchanged since import; it does not prove the original file was authentic.</p>`})}

/* Report */
async function vReport(){
  const n=S.sel.size;
  $('#view').innerHTML=`<div class="panel"><div class="ph"><h3>Investigation report</h3><span class="muted">${n?n+' selected event(s)':'No events selected: all listed events will be used'}</span></div>
  <div class="row"><label>Report title<input id="rt" value="Investigation report"></label><button class="btn pri" id="gen">Generate report</button></div></div><div id="out"></div>`;
  $('#gen').onclick=guard(async()=>{const ev=n?[...S.sel]:S.events.map(e=>e.event_id);
    const {id}=await post('/reports',{title:$('#rt').value,event_ids:ev});const r=await api('/reports/'+encodeURIComponent(id));
    $('#out').innerHTML=`<div class="panel"><div class="ph"><h3>${esc(r.title)}</h3><span class="muted">${esc(r.generated_at)}</span></div>${(r.sections||[]).map(s=>`<h3 style="margin-top:14px">${esc(s.heading)}</h3>${(s.items||[]).map(i=>`<p>${esc(i.text)} <span class="chips">${ids(i.record_ids)}</span></p>`).join('')}`).join('')}<div class="row end"><button class="btn" id="dl">Download</button></div></div>`;
    $('#dl').onclick=()=>{const t=[r.title,r.generated_at,'',...(r.sections||[]).flatMap(s=>[s.heading,...(s.items||[]).map(i=>`- ${i.text} [${(i.record_ids||[]).join(', ')}]`),''])].join('\n');
      const a=document.createElement('a');a.href=URL.createObjectURL(new Blob([t],{type:'text/plain'}));a.download='report-'+id+'.txt';a.click()}})}

/* Import dialog */
const dlg=$('#dlg');
const open=()=>{$('#impres').innerHTML='';dlg.showModal()};
$('#close').onclick=()=>dlg.close();
$('#impf').onsubmit=guard(async e=>{e.preventDefault();
  const fd=new FormData(),f=[...$('#files').files];
  for(const k of ['events','entities','relationships']){const m=f.find(x=>x.name.toLowerCase().includes(k));if(!m)throw new Error('Missing '+k+'.csv');fd.append(k,m)}
  const r=await api('/import',{method:'POST',body:fd});const rej=r.rejected||[];
  $('#impres').innerHTML=`<p><span class="badge ${rej.length?'s2':'ok'}">Imported</span> ${esc(r.counts?.events??0)} events, ${esc(r.counts?.entities??0)} entities, ${esc(r.counts?.relationships??0)} relationships.</p>${rej.length?`<p><b>${rej.length} rejected row(s)</b></p><table><tbody>${rej.map(x=>`<tr><td>${esc(x.file)}</td><td>Line ${esc(x.line)}</td><td class="muted">${esc(x.reason)}</td></tr>`).join('')}</tbody></table>`:''}`;
  await refresh();if(!rej.length)dlg.close()});

/* Global events */
document.addEventListener('click',e=>{
  const t=e.target.closest('[data-tab],[data-go],[data-imp],#imp');if(!t)return;
  if(t.dataset.tab)show(t.dataset.tab);
  else if(t.dataset.go)show(t.dataset.go,t.dataset.id);
  else open()});
document.addEventListener('change',e=>{const s=e.target.dataset?.sel;if(s)e.target.checked?S.sel.add(s):S.sel.delete(s)});
let st;$('#q').oninput=e=>{clearTimeout(st);const v=e.target.value.trim(),ul=$('#sug');
  if(!v||!S.loaded){ul.hidden=true;return}
  st=setTimeout(guard(async()=>{const r=await api('/search?prefix='+encodeURIComponent(v));
    ul.innerHTML=(r.results||[]).map(x=>`<li data-id="${esc(x.id)}">${esc(x.label||x.id)}</li>`).join('')||'<li class="muted">No matches</li>';ul.hidden=false}),200)};
$('#sug').onclick=e=>{const id=e.target.dataset.id;if(id){$('#sug').hidden=true;$('#q').value='';show('graph',id)}};
tabs();refresh();

$('#theme').onclick=()=>{const r=document.documentElement,d=r.dataset.theme?r.dataset.theme==='dark':matchMedia('(prefers-color-scheme:dark)').matches;r.dataset.theme=d?'light':'dark';try{localStorage.setItem('ct_theme',r.dataset.theme)}catch(e){}};
try{const t=localStorage.getItem('ct_theme');if(t)document.documentElement.dataset.theme=t}catch(e){}
