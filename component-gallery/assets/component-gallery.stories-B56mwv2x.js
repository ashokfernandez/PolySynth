const a=()=>{const t=document.createElement("div");t.style.width="1024px",t.style.height="768px",t.style.margin="0 auto",t.style.background="rgb(70, 70, 70)";const e=document.createElement("iframe");return e.title="Component Gallery",e.src="gallery/index.html",e.style.width="100%",e.style.height="100%",e.style.border="0",e.addEventListener("load",()=>{const n=e.contentDocument;if(!n)return;const o=n.getElementById("buttons");o&&(o.style.display="none");const s=n.getElementById("greyout");s&&(s.style.display="none")}),t.appendChild(e),t},l={title:"ComponentGallery/UI Components"},r={render:()=>a(),parameters:{controls:{disable:!0}}};r.parameters={...r.parameters,docs:{...r.parameters?.docs,source:{originalSource:`{
  render: () => createGalleryFrame(),
  parameters: {
    controls: {
      disable: true
    }
  }
}`,...r.parameters?.docs?.source}}};const c=["Knob"];export{r as Knob,c as __namedExportsOrder,l as default};
