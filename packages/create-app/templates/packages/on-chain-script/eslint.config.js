import { defineConfig } from 'eslint/config';
import ckbJsStd from '@ckb-js-std/eslint-plugin';

export default defineConfig([
  {
    files: ['**/*.ts'],
    plugins: { ckbJsStd },
    extends: ['ckbJsStd/recommended'],
  },
]);
