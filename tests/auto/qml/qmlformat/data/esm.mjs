//Imports
import defaultExport from "module-name";
import * as name from "module-name"
import {export1} from "module-name";
import { export1 as alias1 } from "module-name";
import { default as alias } from "module-name"
import { export1, export2 } from "module-name";
import {export1,export2 as alias2} from "module-name";
import defaultExport,{export1,a} from "module-name"
import defaultExport, * as name from "module-name";
import "module-name";


// Exporting declarations
export let name1,name2; // also var
export const name1=1,name2=2 // also var, let
export function functionName() {}
export class ClassName{constructor(h){this.h=h;}}
export function* generatorFunctionName() {}
export const {name1, name2: bar}=o;export const [name1,name2]=array

// Export list
export {name1,nameN};export {variable1 as name1,variable2 as name2,nameN }
export {name1 as default};

// Default exports
export default function* generatorFunctionName() {return 1;}

// Aggregating modules
export * from "module-name";
export { name1,nameN} from "module-name"
export { import1 as name1, import2 as name2,nameN } from "module-name";export { default, } from "module-name";
export { default as name1 } from "module-name";
