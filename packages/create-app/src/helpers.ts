import validateProjectName from 'validate-npm-package-name';
import fs from 'fs-extra';
import spawn from 'cross-spawn';

interface ValidationResult {
  valid: boolean;
  problems?: string[];
}

export function validateNpmName(name: string): ValidationResult {
  const nameValidation = validateProjectName(name);
  if (nameValidation.validForNewPackages) {
    return { valid: true };
  }

  return {
    valid: false,
    problems: [
      ...(nameValidation.errors || []),
      ...(nameValidation.warnings || []),
    ],
  };
}

export function getPkgManager(): string {
  return 'pnpm';
}

export function isFolderEmpty(folderPath: string): boolean {
  const files = fs.readdirSync(folderPath);
  return files.length === 0;
}

export async function install(packageManager: string): Promise<void> {
  const args = ['install'];

  return new Promise((resolve, reject) => {
    const child = spawn(packageManager, args, {
      stdio: 'inherit',
      env: {
        ...process.env,
        ADBLOCK: '1',
        // we set NODE_ENV to development as pnpm skips dev
        // dependencies when production
        NODE_ENV: 'development',
        DISABLE_OPENCOLLECTIVE: '1',
      },
    });
    child.on('close', (code) => {
      if (code !== 0) {
        reject({ command: `${packageManager} ${args.join(' ')}` });
        return;
      }
      resolve();
    });
  });
}